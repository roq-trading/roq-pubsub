/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/market_data.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/charconv.hpp"

#include "roq/core/tools/exception.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/socket/client_factory.hpp"

#include "roq/bybit/flags.hpp"

#include "roq/bybit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {

// === CONSTANTS ===

namespace {
auto const NAME = "md"sv;

auto const SUPPORTS = Mask{
    SupportType::MARKET_STATUS,
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &context, auto api) {
  auto uri = [&]() {
    auto base = flags::Flags::ws_public_uri();
    switch (api) {
      using enum tools::API;
      case UNDEFINED:
        break;
      case SPOT:
        return base.append("/spot"sv);
      case LINEAR:
        return base.append("/linear"sv);
      case INVERSE:
        return base.append("/inverse"sv);
      case OPTION:
        return base.append("/option"sv);
    }
    log::fatal("Unexpected"sv);
  }();
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .validate_certificate = server::Flags::net_tls_validate_certificate(),
      // connection manager
      .connection_timeout = server::Flags::net_connection_timeout(),
      .disconnect_on_idle_timeout = server::Flags::net_disconnect_on_idle_timeout(),
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = Flags::ws_ping_freq(),
      // implementation
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
  };
  return web::socket::ClientFactory::create(handler, context, config, []() { return std::string(); });
}

auto is_spot(auto api) {
  return api == tools::API::SPOT;
}

auto get_mbp_depth(auto api) -> size_t {
  auto result = flags::Flags::ws_mbp_depth();
  if (!result) {
    switch (api) {
      using enum tools::API;
      case UNDEFINED:
        break;
      case SPOT:
        return 50;
      case LINEAR:
        return 50;
      case INVERSE:
        return 50;
      case OPTION:
        return 25;
    }
    log::fatal("Unexpected"sv);
  }
  return result;
}

auto create_mbp_topic(size_t depth) {
  return fmt::format("orderbook.{}"sv, depth);
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto const &group, auto const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

// === IMPLEMENTATION ===

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index},
      ping_frequency_{Flags::ws_ping_freq()}, spot_{is_spot(shared.api)}, mbp_depth_{get_mbp_depth(shared.api)},
      mbp_topic_{create_mbp_topic(mbp_depth_)}, connection_{create_connection(*this, context, shared.api)},
      decode_buffer_{Flags::decode_buffer_size()},
      request_id_{static_cast<uint64_t>(stream_id_) * 1000000},  // scale (debugging)
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .order_book = create_metrics(name_, "order_book"sv),
          .trade = create_metrics(name_, "trade"sv),
          .tickers = create_metrics(name_, "tickers"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
      },
      shared_{shared} {
}

void MarketData::operator()(Event<Start> const &) {
  (*connection_).start();
}

void MarketData::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void MarketData::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if (ready() && next_ping_ < now)
    send_ping(now);
}

void MarketData::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.order_book, metrics::PROFILE)
      .write(profile_.trade, metrics::PROFILE)
      .write(profile_.tickers, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
}

void MarketData::operator()(web::socket::Client::Connected const &) {
}

void MarketData::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void MarketData::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::READY);
  subscribe();
}

void MarketData::operator()(web::socket::Client::Close const &) {
}

void MarketData::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(web::socket::Client::Text const &text) {
  parse(text.payload);
}

void MarketData::operator()(web::socket::Client::Binary const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::WS,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
        .interface = (*connection_).get_interface(),
        .authority = (*connection_).get_current_authority(),
        .path = (*connection_).get_current_path(),
        .proxy = (*connection_).get_proxy(),
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols))
    return;
  if (spot_)
    subscribe("orderbook.1"sv, symbols);
  subscribe(mbp_topic_, symbols);
  subscribe("publicTrade"sv, symbols);
  subscribe("tickers"sv, symbols);
}

void MarketData::subscribe(std::string_view const &topic, std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  auto separator = fmt::format(R"(","{}.)"sv, topic);
  auto message = fmt::format(
      R"({{)"
      R"("req_id":"{}",)"
      R"("op":"subscribe",)"
      R"("args":["{}.{}"])"
      R"(}})"sv,
      ++request_id_,
      topic,
      fmt::join(symbols, separator));
  log::debug("message={}"sv, message);
  (*connection_).send_text(message);
}

void MarketData::send_ping(std::chrono::nanoseconds now) {
  assert(ping_frequency_.count() > 0);
  next_ping_ = now + ping_frequency_ / 2;
  auto message = fmt::format(
      R"({{)"
      R"("req_id":"{}",)"
      R"("op":"ping")"
      R"(}})"sv,
      now.count());
  // log::debug("message={}"sv, message);
  (*connection_).send_text(message);
}

void MarketData::parse(std::string_view const &message) {
  profile_.parse([&]() {
    try {
      TraceInfo trace_info;
      core::json::Buffer buffer{decode_buffer_};
      if (json::Parser::dispatch(*this, message, buffer, trace_info)) {
      } else {
        log::fatal(R"(Unexpected: failed to parse message="{}")"sv, message);
      }
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void MarketData::operator()(Trace<json::Ping> const &event) {
  auto &[trace_info, ping] = event;
  log::info<4>("event={{ping={}, trace_info={}}}"sv, ping, trace_info);
}

void MarketData::operator()(Trace<json::Auth> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::Subscribe> const &event) {
  auto &[trace_info, subscribe] = event;
  log::info<4>("event={{subscribe={}, trace_info={}}}"sv, subscribe, trace_info);
}

void MarketData::operator()(Trace<json::Error> const &event) {
  auto &[trace_info, error] = event;
  log::info<4>("event={{error={}, trace_info={}}}"sv, error, trace_info);
  log::fatal("error={}"sv, error);
}

void MarketData::operator()(Trace<json::OrderBook> const &event, size_t depth) {
  profile_.order_book([&]() {
    auto &[trace_info, order_book] = event;
    log::info<3>("event={{order_book={}, trace_info={}}}"sv, order_book, trace_info);
    // log::debug("order_book={}"sv, order_book);
    (*connection_).touch(trace_info.source_receive_time);
    auto update_type = json::map(order_book.type);
    auto &data = order_book.data;
    if (depth == 1) {
      auto helper = [](auto &levels) -> std::pair<double, double> {
        double price = NaN, quantity = NaN;
        // first non-zero quantity
        for (auto &item : levels) {
          if (utils::compare(item.quantity, 0.0) > 0) {
            price = item.price;
            quantity = item.quantity;
            break;
          }
        }
        return {price, quantity};
      };
      auto [bid_price, bid_quantity] = helper(data.bids);
      auto [ask_price, ask_quantity] = helper(data.asks);
      auto top_of_book = TopOfBook{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = data.symbol,
          .layer{
              .bid_price = bid_price,
              .bid_quantity = bid_quantity,
              .ask_price = ask_price,
              .ask_quantity = ask_quantity,
          },
          .update_type = update_type,
          .exchange_time_utc = order_book.timestamp,
          .exchange_sequence = data.cross_sequence,
          .sending_time_utc = {},
      };
      create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
    } else {
      shared_.bids.clear();
      shared_.asks.clear();
      auto emplace_back = [](auto &result, auto &item) {
        auto mbp_update = MBPUpdate{
            .price = item.price,
            .quantity = item.quantity,
            .implied_quantity = NaN,
            .number_of_orders = {},
            .update_action = {},
            .price_level = {},
        };
        result.emplace_back(std::move(mbp_update));
      };
      for (auto &item : data.bids)
        emplace_back(shared_.bids, item);
      for (auto &item : data.asks)
        emplace_back(shared_.asks, item);
      auto market_by_price_update = MarketByPriceUpdate{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = data.symbol,
          .bids = shared_.bids,
          .asks = shared_.asks,
          .update_type = update_type,
          .exchange_time_utc = order_book.timestamp,
          .exchange_sequence = data.cross_sequence,
          .sending_time_utc = {},
          .price_decimals = {},
          .quantity_decimals = {},
          .checksum = {},
      };
      try {
        create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true);
      } catch (BadState &) {
        // resubscribe(symbol);
      }
    }
  });
}

void MarketData::operator()(Trace<json::PublicTrade> const &event) {
  profile_.trade([&]() {
    auto &[trace_info, public_trade] = event;
    log::info<3>("event={{public_trade={}, trace_info={}}}"sv, public_trade, trace_info);
    // log::debug("public_trade={}"sv, public_trade);
    (*connection_).touch(trace_info.source_receive_time);
    auto &trades = shared_.trades;
    trades.clear();
    std::chrono::milliseconds timestamp = {};
    auto dispatch = [&](auto &symbol) {
      if (std::empty(symbol)) {
        assert(std::empty(trades));
      }
      if (std::empty(trades))
        return;
      auto trade_summary = TradeSummary{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = symbol,
          .trades = trades,
          .exchange_time_utc = timestamp,
          .exchange_sequence = {},
          .sending_time_utc = public_trade.timestamp,
      };
      create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
      trades.clear();
      timestamp = {};
    };
    std::string_view previous;
    for (auto &item : public_trade.data) {
      if (item.symbol != previous) {
        dispatch(previous);
        previous = item.symbol;
      }
      auto side = json::map(item.side);
      auto trade_2 = Trade{
          .side = side,
          .price = item.price,
          .quantity = item.quantity,
          .trade_id = item.trade_id,
          .taker_order_id = {},
          .maker_order_id = {},
      };
      trades.emplace_back(std::move(trade_2));
      utils::update_max(timestamp, item.timestamp);
    }
    dispatch(previous);
  });
}

void MarketData::operator()(Trace<json::Tickers> const &event) {
  profile_.tickers([&]() {
    auto &[trace_info, tickers] = event;
    log::info<3>("event={{tickers={}, trace_info={}}}"sv, tickers, trace_info);
    // log::debug("tickers={}"sv, tickers);
    (*connection_).touch(trace_info.source_receive_time);
    auto &data = tickers.data;
    switch (shared_.api) {
      using enum tools::API;
      case UNDEFINED:
        break;
      case SPOT:
        // note! using orderbook.1
        break;
      case LINEAR:
      case INVERSE: {
        auto top_of_book = TopOfBook{
            .stream_id = stream_id_,
            .exchange = Flags::exchange(),
            .symbol = data.symbol,
            .layer{
                .bid_price = data.bid1_price,
                .bid_quantity = data.bid1_size,
                .ask_price = data.ask1_price,
                .ask_quantity = data.ask1_size,
            },
            .update_type = UpdateType::INCREMENTAL,
            .exchange_time_utc = {},
            .exchange_sequence = tickers.cross_sequence,
            .sending_time_utc = tickers.timestamp,
        };
        create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
        break;
      }
      case OPTION: {
        auto top_of_book = TopOfBook{
            .stream_id = stream_id_,
            .exchange = Flags::exchange(),
            .symbol = data.symbol,
            .layer{
                .bid_price = data.bid_price,
                .bid_quantity = data.bid_size,
                .ask_price = data.ask_price,
                .ask_quantity = data.ask_size,
            },
            .update_type = UpdateType::INCREMENTAL,
            .exchange_time_utc = {},
            .exchange_sequence = tickers.cross_sequence,
            .sending_time_utc = tickers.timestamp,
        };
        create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
        break;
      }
    }
    auto statistics = std::array<Statistics, 4>{{
        {
            .type = StatisticsType::HIGHEST_TRADED_PRICE,
            .value = data.high_price24h,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::LOWEST_TRADED_PRICE,
            .value = data.low_price24h,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::CLOSE_PRICE,
            .value = data.last_price,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::TRADE_VOLUME,
            .value = data.volume24h,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
    }};
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = data.symbol,
        .statistics = statistics,
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = {},
        .exchange_sequence = tickers.cross_sequence,
        .sending_time_utc = tickers.timestamp,
    };
    create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
  });
}

void MarketData::operator()(Trace<json::Wallet> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::Position> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::Order> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::Execution2> const &) {
  log::fatal("Unexpected"sv);
}

}  // namespace bybit
}  // namespace roq
