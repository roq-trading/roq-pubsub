/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/rest.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/rest/client_factory.hpp"

#include "roq/bybit/flags.hpp"

#include "roq/bybit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {

// === TODO ===
// => use rate limiter / request queue
// => query instrument-info every N seconds

// === CONSTANTS ===

namespace {
auto const NAME = "rest"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::rest_uri();
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .validate_certificate = server::Flags::net_tls_validate_certificate(),
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // proxy
      .proxy = Flags::rest_proxy(),
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = Flags::rest_request_timeout(),
      .ping_frequency = Flags::rest_ping_freq(),
      .ping_path = Flags::rest_ping_path(),
      // implementation
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
      .allow_pipelining = true,
  };
  return web::rest::ClientFactory::create(handler, context, config);
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto const &group, auto const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

// === IMPLEMENTATION ===

Rest::Rest(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)},
      connection_{create_connection(*this, context)}, decode_buffer_{Flags::decode_buffer_size()},
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .instrument_info = create_metrics(name_, "instrument_info"sv),
          .instrument_info_ack = create_metrics(name_, "instrument_info_ack"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      shared_{shared}, download_{Flags::rest_request_timeout(), [this](auto state) { return download(state); }},
      rate_limiter{flags::Flags::request_limit(), flags::Flags::request_limit_interval()} {
}

void Rest::operator()(Event<Start> const &) {
  (*connection_).start();
}

void Rest::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void Rest::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
}

void Rest::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.instrument_info, metrics::PROFILE)
      .write(profile_.instrument_info_ack, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

void Rest::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
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

void Rest::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void Rest::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void Rest::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void Rest::operator()(
    Trace<web::rest::Response> const &, [[maybe_unused]] uint64_t request_id, [[maybe_unused]] uint64_t opaque) {
}

uint32_t Rest::download(RestState state) {
  switch (state) {
    using enum RestState;
    case UNDEFINED:
      assert(false);
      break;
    case GET_INSTRUMENT_INFO:
      get_instrument_info();
      return 1;
    case DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// market info

void Rest::get_instrument_info() {
  profile_.instrument_info([&]() {
    auto query = fmt::format(
        "?category={}"
        "&status=Trading"
        "&limit=1000"sv,
        shared_.category.as_raw_text());
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = "/v5/market/instruments-info"sv,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_instrument_info_ack(event, sequence);
    };
    (*connection_)("market_info_spot"sv, request, callback);
  });
}

void Rest::get_instrument_info_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = RestState::GET_INSTRUMENT_INFO;
  profile_.instrument_info_ack([&]() {
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::InstrumentInfo instrument_info{body, decode_buffer_};
        Trace event_2{event, instrument_info};
        (*this)(event_2);
        // XXX HANS NEW ??? create_trace_and_dispatch(*this, event, instrument_info)();
        download_.check(STATE);
      }
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      download_.retry(STATE);
    };
    process_response(event, handle_success, handle_error);
  });
}

void Rest::operator()(Trace<json::InstrumentInfo> const &event) {
  auto &[trace_info, instrument_info] = event;
  log::info<4>("instrument_info={}"sv, instrument_info);
  std::vector<Symbol> symbols;
  symbols.reserve(std::size(instrument_info.result.list));  // alloc
  size_t counter = 0;
  for (auto &item : instrument_info.result.list) {
    log::info<2>("item={}"sv, item);
    auto discard = shared_.discard_symbol(item.symbol);
    auto security_type = json::map(item.contract_type, item.options_type);
    auto option_type = json::map(item.options_type);
    auto reference_data = ReferenceData{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = item.symbol,
        .description = item.symbol,
        .security_type = security_type,
        .base_currency = item.base_coin,
        .quote_currency = item.quote_coin,
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = item.price_filter.tick_size,
        .multiplier = 1.0,
        .min_notional = NaN,
        .min_trade_vol = item.lot_size_filter.min_order_qty,
        .max_trade_vol = item.lot_size_filter.max_order_qty,
        .trade_vol_step_size = NaN,
        .option_type = option_type,
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
        .discard = discard,
    };
    create_trace_and_dispatch(handler_, trace_info, reference_data, true);
    if (discard)
      continue;
    if (symbols_.emplace(item.symbol).second)  // only include new
      symbols.emplace_back(item.symbol);
    ++counter;
    auto trading_status = json::map(item.status);
    auto market_status = MarketStatus{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = item.symbol,
        .trading_status = trading_status,
    };
    create_trace_and_dispatch(handler_, trace_info, market_status, true);
  }
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
  if (counter > 0)
    log::info("Symbols {} / {}"sv, counter, std::size(instrument_info.result.list));
}

template <typename SuccessHandler, typename ErrorHandler>
void Rest::process_response(
    web::rest::Response const &response, SuccessHandler success_handler, ErrorHandler error_handler) {
  try {
    auto [status, category, body] = response.result();
    // log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
    switch (category) {
      using enum web::http::Category;
      case SUCCESS:  // 2xx
        success_handler(body);
        break;
      case CLIENT_ERROR: {  // 4xx
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, text);
        break;
      }
      case SERVER_ERROR: {  // 5xx
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, text);
        break;
      }
      default:
        response.expect(web::http::Status::OK);  // throws
    }
  } catch (oms::Exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(e.origin, e.status, e.error, e.what());
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

}  // namespace bybit
}  // namespace roq
