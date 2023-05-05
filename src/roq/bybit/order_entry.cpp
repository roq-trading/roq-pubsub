/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/order_entry.hpp"

#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/number.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/rest/client_factory.hpp"

#include "roq/bybit/flags.hpp"

#include "roq/bybit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {

// === CONSTANTS ===

namespace {
auto const NAME = "om"sv;
}  // namespace

// === HELPERS ===

namespace {
auto get_supports(auto api) {
  auto result = Mask{
      SupportType::CREATE_ORDER,
      SupportType::CANCEL_ORDER,
      SupportType::ORDER_ACK,
      SupportType::FUNDS,
  };
  if (api != tools::API::SPOT)
    result |= SupportType::MODIFY_ORDER;
  return result;
}
}  // namespace

// === CONSTANTS ===

namespace {
auto create_name(auto stream_id, auto const &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account);
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

OrderEntry::OrderEntry(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account.get_name())},
      connection_{create_connection(*this, context)}, decode_buffer_{Flags::decode_buffer_size()},
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .account_info = create_metrics(name_, "account_info"sv),
          .account_info_ack = create_metrics(name_, "account_info_ack"sv),
          .wallet_balance = create_metrics(name_, "wallet_balance"sv),
          .wallet_balance_ack = create_metrics(name_, "wallet_balance_ack"sv),
          .position_info = create_metrics(name_, "position_info"sv),
          .position_info_ack = create_metrics(name_, "position_info_ack"sv),
          .open_orders = create_metrics(name_, "open_orders"sv),
          .open_orders_ack = create_metrics(name_, "open_orders_ack"sv),
          .execution = create_metrics(name_, "execution"sv),
          .execution_ack = create_metrics(name_, "execution_ack"sv),
          .place_order = create_metrics(name_, "place_order"sv),
          .place_order_ack = create_metrics(name_, "place_order_ack"sv),
          .amend_order = create_metrics(name_, "amend_order"sv),
          .amend_order_ack = create_metrics(name_, "amend_order_ack"sv),
          .cancel_order = create_metrics(name_, "cancel_order"sv),
          .cancel_order_ack = create_metrics(name_, "cancel_order_ack"sv),
          .cancel_all_orders = create_metrics(name_, "cancel_all_orders"sv),
          .cancel_all_orders_ack = create_metrics(name_, "cancel_all_orders_ack"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      account_{account}, shared_{shared},
      download_{Flags::rest_request_timeout(), [this](auto state) { return download(state); }} {
}

void OrderEntry::operator()(Event<Start> const &) {
  (*connection_).start();
}

void OrderEntry::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void OrderEntry::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if (ready())
    check_request_queue(now);
}

void OrderEntry::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.account_info, metrics::PROFILE)
      .write(profile_.account_info_ack, metrics::PROFILE)
      .write(profile_.wallet_balance, metrics::PROFILE)
      .write(profile_.wallet_balance_ack, metrics::PROFILE)
      .write(profile_.position_info, metrics::PROFILE)
      .write(profile_.position_info_ack, metrics::PROFILE)
      .write(profile_.open_orders, metrics::PROFILE)
      .write(profile_.open_orders_ack, metrics::PROFILE)
      .write(profile_.execution, metrics::PROFILE)
      .write(profile_.execution_ack, metrics::PROFILE)
      .write(profile_.place_order, metrics::PROFILE)
      .write(profile_.place_order_ack, metrics::PROFILE)
      .write(profile_.amend_order, metrics::PROFILE)
      .write(profile_.amend_order_ack, metrics::PROFILE)
      .write(profile_.cancel_order, metrics::PROFILE)
      .write(profile_.cancel_order_ack, metrics::PROFILE)
      .write(profile_.cancel_all_orders, metrics::PROFILE)
      .write(profile_.cancel_all_orders_ack, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

uint16_t OrderEntry::operator()(
    Event<CreateOrder> const &event, oms::Order const &order, std::string_view const &request_id) {
  place_order(event, order, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    Event<ModifyOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  amend_order(event, order, request_id, previous_request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    Event<CancelOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  cancel_order(event, order, request_id, previous_request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  cancel_all_orders(event, request_id);
  return stream_id_;
}

void OrderEntry::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void OrderEntry::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void OrderEntry::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.get_name(),
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void OrderEntry::operator()(
    Trace<web::rest::Response> const &, [[maybe_unused]] uint64_t request_id, [[maybe_unused]] uint64_t opaque) {
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = account_.get_name(),
        .supports = get_supports(shared_.api),
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

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    using enum OrderEntryState;
    case UNDEFINED:
      assert(false);
      break;
    case ACCOUNT_INFO:
      get_account_info();
      return 1;
    case DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

void OrderEntry::check_request_queue(std::chrono::nanoseconds now) {
  auto request = [&](auto &message) {
    auto &[topic, symbol] = message;
    if (topic.compare("wallet"sv) == 0) {
      get_wallet_balance();
    } else if (topic.compare("position"sv) == 0) {
      get_position_info(symbol);
    } else if (topic.compare("order"sv) == 0) {
      get_open_orders(symbol);
    } else if (topic.compare("execution"sv) == 0) {
      get_execution(symbol);
    }
  };
  account_.request_queue.dispatch(now, request);
}

void OrderEntry::get_account_info() {
  profile_.account_info([&]() {
    auto const path = "/v5/account/info"sv;
    auto headers = account_.create_headers(path, {}, {});
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_account_info_ack(event, sequence);
    };
    (*connection_)("account_info"sv, request, callback);
  });
}

void OrderEntry::get_account_info_ack(Trace<web::rest::Response> const &event, [[maybe_unused]] uint32_t sequence) {
  auto const constexpr STATE = OrderEntryState::ACCOUNT_INFO;
  profile_.account_info_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::AccountInfo account_info{body, decode_buffer_};
      log::debug("account_info={}"sv, account_info);
      Trace event_2{event, account_info};
      (*this)(event_2);
      download_.check_relaxed(STATE);
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      if (download_.downloading())
        download_.retry(STATE);
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<json::AccountInfo> const &event) {
  auto &[trace_info, account_info] = event;
  log::info<2>("account_info={}"sv, account_info);
  // XXX HANS maybe do something with unified account ???
}

void OrderEntry::get_wallet_balance() {
  profile_.wallet_balance([&]() {
    auto path = "/v5/account/wallet-balance"sv;
    auto account_type = [&]() -> std::string_view {
      switch (shared_.api) {
        using enum tools::API;
        case UNDEFINED:
          break;
        case SPOT:
          return "SPOT"sv;
        case LINEAR:
          return "UNIFIED"sv;
        case INVERSE:
          return "CONTRACT"sv;
        case OPTION:
          return "UNIFIED"sv;
      }
      log::fatal("Unexpected"sv);
    }();
    auto query = fmt::format("?accountType={}"sv, account_type);
    auto headers = account_.create_headers(path, query, {});
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_wallet_balance_ack(event);
    };
    (*connection_)("wallet_balance"sv, request, callback);
  });
}

void OrderEntry::get_wallet_balance_ack(Trace<web::rest::Response> const &event) {
  profile_.wallet_balance_ack([&]() {
    if (event.value.status() == web::http::Status::NOT_FOUND) {
      return;
    }
    auto handle_success = [&](auto &body) {
      if (json::WalletParser::dispatch(*this, body, decode_buffer_, event)) {
      } else {
        log::warn(R"(Unexpected: message="{}")"sv, body);
      }
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
    };
    process_response(event, handle_success, handle_error);
    auto response = Response{
        .account = account_.get_name(),
        .topic = "wallet"sv,
        .symbol = {},
    };
    create_trace_and_dispatch(handler_, event, response);
  });
}

void OrderEntry::operator()(Trace<json::Wallet> const &event) {
  auto &[trace_info, wallet] = event;
  log::info<2>("wallet={}"sv, wallet);
  for (auto &item : wallet.coin) {
    log::debug("item={}"sv, item);
    auto funds_update = FundsUpdate{
        .stream_id = stream_id_,
        .account = account_.get_name(),
        .currency = item.coin,
        .balance = item.wallet_balance,  // XXX item.free ???
        .hold = item.locked,
        .external_account = {},
        .update_type = UpdateType::SNAPSHOT,
        .exchange_time_utc = {},
        .sending_time_utc = {},  // XXX lost when flattened
    };
    create_trace_and_dispatch(handler_, trace_info, funds_update, true);
  }
}

void OrderEntry::get_position_info(std::string_view const &symbol) {
  profile_.position_info([&]() {
    assert(shared_.api != tools::API::SPOT);
    auto const path = "/v5/position/list"sv;
    auto category = [&]() -> std::string_view {
      switch (shared_.api) {
        using enum tools::API;
        case UNDEFINED:
          break;
        case SPOT:
          break;
        case LINEAR:
          return "linear"sv;
        case INVERSE:
          return "inverse"sv;
        case OPTION:
          return "option"sv;
      }
      log::fatal("Unexpected"sv);
    }();
    auto query = fmt::format("?category={}&symbol={}&limit=200"sv, category, symbol);
    auto headers = account_.create_headers(path, query, {});
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, symbol = std::string{symbol}]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_position_info_ack(event, symbol);
    };
    (*connection_)("position_info"sv, request, callback);
  });
}

void OrderEntry::get_position_info_ack(Trace<web::rest::Response> const &event, std::string_view const &symbol) {
  profile_.position_info_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::PositionInfo position_info{body, decode_buffer_};
      log::debug("position_info={}"sv, position_info);
      Trace event_2{event, position_info};
      (*this)(event_2);
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
    };
    process_response(event, handle_success, handle_error);
    auto response = Response{
        .account = account_.get_name(),
        .topic = "position"sv,
        .symbol = symbol,
    };
    create_trace_and_dispatch(handler_, event, response);
  });
}

void OrderEntry::operator()(Trace<json::PositionInfo> const &event) {
  auto &[trace_info, position_info] = event;
  log::info<2>("position_info={}"sv, position_info);
  for (auto &item : position_info.result.list) {
    if (shared_.discard_symbol(item.symbol))
      continue;
    // log::debug("item={}"sv, item);
    auto side = json::map(item.side);
    auto quantity = utils::sign(side) * item.size;
    auto long_quantity = std::max(0.0, quantity);
    auto short_quantity = std::max(0.0, -quantity);
    auto position_update = PositionUpdate{
        .stream_id = stream_id_,
        .account = account_.get_name(),
        .exchange = Flags::exchange(),
        .symbol = item.symbol,
        .external_account = {},
        .long_quantity = long_quantity,
        .short_quantity = short_quantity,
        .update_type = UpdateType::SNAPSHOT,
        .exchange_time_utc = item.updated_time,  // XXX created_time ???
        .sending_time_utc = position_info.time,
    };
    create_trace_and_dispatch(handler_, trace_info, position_update, true);
  }
}

void OrderEntry::get_open_orders(std::string_view const &symbol) {
  profile_.open_orders([&]() {
    auto path = "/v5/order/realtime"sv;
    auto category = [&]() -> std::string_view {
      switch (shared_.api) {
        using enum tools::API;
        case UNDEFINED:
          break;
        case SPOT:
          return "spot"sv;
        case LINEAR:
          return "linear"sv;
        case INVERSE:
          return "inverse"sv;
        case OPTION:
          return "option"sv;
      }
      log::fatal("Unexpected"sv);
    }();
    auto query = fmt::format("?category={}&symbol={}&openOnly=0&limit=50"sv, category, symbol);
    auto headers = account_.create_headers(path, query, {});
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, symbol = std::string{symbol}]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_open_orders_ack(event, symbol);
    };
    (*connection_)("open_orders"sv, request, callback);
  });
}

void OrderEntry::get_open_orders_ack(Trace<web::rest::Response> const &event, std::string_view const &symbol) {
  profile_.open_orders_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::OpenOrders open_orders{body, decode_buffer_};
      log::debug("open_orders={}"sv, open_orders);
      Trace event_2{event, open_orders};
      (*this)(event_2);
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
    };
    process_response(event, handle_success, handle_error);
    auto response = Response{
        .account = account_.get_name(),
        .topic = "order"sv,
        .symbol = symbol,
    };
    create_trace_and_dispatch(handler_, event, response);
  });
}

void OrderEntry::operator()(Trace<json::OpenOrders> const &event) {
  auto &[trace_info, open_orders] = event;
  log::info<2>("open_orders={}"sv, open_orders);
  for (auto &item : open_orders.result.list) {
    log::debug("item={}"sv, item);
    auto side = json::map(item.side);
    auto order_type = json::map(item.order_type);
    auto time_in_force = json::map(item.time_in_force);
    auto order_status = json::map(item.order_status);
    auto order_update = oms::OrderUpdate{
        .account = account_.get_name(),
        .exchange = Flags::exchange(),
        .symbol = item.symbol,
        .side = side,
        .position_effect = {},
        .max_show_quantity = NaN,
        .order_type = order_type,
        .time_in_force = time_in_force,
        .execution_instructions = {},
        .create_time_utc = item.created_time,
        .update_time_utc = item.updated_time,
        .external_account = {},
        .external_order_id = item.order_id,
        .status = order_status,
        .quantity = item.qty,
        .price = item.price,
        .stop_price = NaN,  // XXX item.trigger_price ???
        .remaining_quantity = item.leaves_qty,
        .traded_quantity = item.cum_exec_qty,
        .average_traded_price = item.avg_price,
        .last_traded_quantity = NaN,
        .last_traded_price = NaN,
        .last_liquidity = {},
        .update_type = UpdateType::SNAPSHOT,
        .sending_time_utc = open_orders.time,
    };
    Trace event_2{trace_info, order_update};
    (*this)(event_2, item.order_link_id);
  }
}

void OrderEntry::get_execution(std::string_view const &symbol) {
  profile_.execution([&]() {
    auto path = "/v5/execution/list"sv;
    auto category = [&]() -> std::string_view {
      switch (shared_.api) {
        using enum tools::API;
        case UNDEFINED:
          break;
        case SPOT:
          return "spot"sv;
        case LINEAR:
          return "linear"sv;
        case INVERSE:
          return "inverse"sv;
        case OPTION:
          return "option"sv;
      }
      log::fatal("Unexpected"sv);
    }();
    auto end_time = clock::get_realtime<std::chrono::milliseconds>();
    auto start_time = end_time - flags::Flags::execution_lookback();
    auto query = fmt::format("?category={}&symbol={}&startTime={}&limit=100"sv, category, symbol, start_time.count());
    auto headers = account_.create_headers(path, query, {});
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, symbol = std::string{symbol}]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_execution_ack(event, symbol);
    };
    (*connection_)("execution"sv, request, callback);
  });
}

void OrderEntry::get_execution_ack(Trace<web::rest::Response> const &event, std::string_view const &symbol) {
  profile_.execution_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::Execution execution{body, decode_buffer_};
      log::debug("execution={}"sv, execution);
      Trace event_2{event, execution};
      (*this)(event_2);
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
    };
    process_response(event, handle_success, handle_error);
    auto response = Response{
        .account = account_.get_name(),
        .topic = "execution"sv,
        .symbol = symbol,
    };
    create_trace_and_dispatch(handler_, event, response);
  });
}

void OrderEntry::operator()(Trace<json::Execution> const &event) {
  auto &trace_info = event.trace_info;
  auto &execution = event.value;
  log::info<2>("execution={}"sv, execution);
  std::string_view order_id, order_link_id, symbol;
  Side side = {};
  std::chrono::nanoseconds exec_time = {};
  auto dispatch = [&]() {
    if (!std::empty(order_link_id)) {
      if (shared_.find_order(order_link_id, [&](auto &order) {
            auto trade_update = oms::TradeUpdate{
                .account = order.account,
                .order_id = order.order_id,
                .exchange = order.exchange,
                .symbol = order.symbol,
                .side = order.side,
                .position_effect = order.position_effect,
                .create_time_utc = utils::safe_cast(exec_time),
                .update_time_utc = utils::safe_cast(exec_time),
                .external_account = {},
                .external_order_id = order_id,
                .fills = shared_.fills,
                .update_type = UpdateType::SNAPSHOT,
                .sending_time_utc = execution.time,
            };
            create_trace_and_dispatch(handler_, trace_info, trade_update, stream_id_, true, order.user_id);
          })) {
      } else {
        auto trade_update = oms::TradeUpdate{
            .account = account_.get_name(),
            .order_id = ORDER_ID_NONE,
            .exchange = flags::Flags::exchange(),
            .symbol = symbol,
            .side = side,
            .position_effect = {},
            .create_time_utc = utils::safe_cast(exec_time),
            .update_time_utc = utils::safe_cast(exec_time),
            .external_account = {},
            .external_order_id = order_id,
            .fills = shared_.fills,
            .update_type = UpdateType::INCREMENTAL,
            .sending_time_utc = execution.time,
        };
        create_trace_and_dispatch(handler_, trace_info, trade_update, stream_id_, true, SOURCE_SELF);
      }
    }
    shared_.fills.clear();
    order_id = {};
    order_link_id = {};
    symbol = {};
    side = {};
    exec_time = {};
  };
  for (auto &item : execution.result.list) {
    if (item.order_id != order_id) {
      dispatch();
      order_id = item.order_id;
      order_link_id = item.order_link_id;
      symbol = item.symbol;
      side = json::map(item.side);
      exec_time = item.exec_time;
    }
    auto liquidity = item.is_maker ? Liquidity::MAKER : Liquidity::TAKER;
    auto fill = Fill{
        .external_trade_id = item.exec_id,
        .quantity = item.exec_qty,
        .price = item.exec_price,
        .liquidity = liquidity,
    };
    shared_.fills.emplace_back(std::move(fill));
  }
  dispatch();
}

void OrderEntry::place_order(
    Event<CreateOrder> const &event, oms::Order const &order, std::string_view const &request_id) {
  profile_.place_order([&]() {
    if (!ready())
      throw oms::NotReady{"not ready"sv};
    auto &[message_info, create_order] = event;
    auto const path = "/v5/order/create"sv;
    std::string buffer;  // XXX
    auto body = json::place_order(buffer, create_order, order, request_id, shared_.category);
    log::debug(R"(body="{}")"sv, body);
    auto headers = account_.create_headers(path, {}, body);
    auto request = web::rest::Request{
        .method = web::http::Method::POST,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = web::http::ContentType::APPLICATION_JSON,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
    };
    auto callback = [this, user_id = message_info.source, order_id = create_order.order_id](
                        [[maybe_unused]] auto &request_id, auto &response) {
      auto version = uint32_t{1};
      TraceInfo trace_info;
      Trace event{trace_info, response};
      place_order_ack(event, user_id, order_id, version);
    };
    (*connection_)("place_order"sv, request, callback);
  });
}

void OrderEntry::place_order_ack(
    Trace<web::rest::Response> const &event, uint8_t user_id, uint32_t order_id, uint32_t version) {
  profile_.place_order_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::PlaceOrder place_order{body, decode_buffer_};
      log::debug("place_order={}"sv, place_order);
      Trace event_2{event, place_order};
      (*this)(event_2, user_id, order_id, version);
    };
    auto handle_error = [&](auto origin, auto status, auto error, auto text) {
      auto response = oms::Response{
          .type = RequestType::CREATE_ORDER,
          .origin = origin,
          .status = status,
          .error = error,
          .text = text,
          .version = version,
          .request_id = {},
          .quantity = NaN,
          .price = NaN,
      };
      Trace event_2{event, response};
      (*this)(event_2, user_id, order_id);
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(
    Trace<json::PlaceOrder> const &event, uint8_t user_id, uint32_t order_id, uint32_t version) {
  auto &[trace_info, place_order] = event;
  log::info<2>("place_order={}"sv, place_order);
  auto request_status = place_order.ret_code == 0 ? RequestStatus::ACCEPTED : RequestStatus::REJECTED;
  auto error = json::map_error(place_order.ret_code);
  auto text = place_order.ret_msg;
  auto &result = place_order.result;
  auto order_status = place_order.ret_code == 0 ? OrderStatus::ACCEPTED : OrderStatus::REJECTED;
  auto response = oms::Response{
      .type = RequestType::CREATE_ORDER,
      .origin = Origin::EXCHANGE,
      .status = request_status,
      .error = error,
      .text = text,
      .version = version,
      .request_id = result.order_link_id,
      .quantity = NaN,
      .price = NaN,
  };
  auto order_update = oms::OrderUpdate{
      .account = account_.get_name(),
      .exchange = Flags::exchange(),
      .symbol = {},
      .side = {},
      .position_effect = {},
      .max_show_quantity = NaN,
      .order_type = {},
      .time_in_force = {},
      .execution_instructions = {},
      .create_time_utc = {},
      .update_time_utc = place_order.time,
      .external_account = {},
      .external_order_id = result.order_id,
      .status = order_status,
      .quantity = NaN,
      .price = NaN,
      .stop_price = NaN,
      .remaining_quantity = NaN,
      .traded_quantity = NaN,
      .average_traded_price = NaN,
      .last_traded_quantity = NaN,
      .last_traded_price = NaN,
      .last_liquidity = {},
      .update_type = UpdateType::INCREMENTAL,
      .sending_time_utc = place_order.time,
  };
  Trace event_2{trace_info, response};
  (*this)(event_2, user_id, order_id, order_update);
}

void OrderEntry::amend_order(
    Event<ModifyOrder> const &event,
    oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  profile_.amend_order([&]() {
    if (shared_.api == tools::API::SPOT)
      throw oms::NotSupported{"amend_order"sv};
    if (!ready())
      throw oms::NotReady{"not ready"sv};
    auto &[message_info, modify_order] = event;
    auto const path = "/v5/order/amend"sv;
    std::string buffer;  // XXX
    auto body = json::amend_order(buffer, modify_order, order, request_id, previous_request_id, shared_.category);
    log::debug(R"(body="{}")"sv, body);
    auto headers = account_.create_headers(path, {}, body);
    auto request = web::rest::Request{
        .method = web::http::Method::POST,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = web::http::ContentType::APPLICATION_JSON,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
    };
    auto callback =
        [this, user_id = message_info.source, order_id = modify_order.order_id, version = modify_order.version](
            [[maybe_unused]] auto &request_id, auto &response) {
          TraceInfo trace_info;
          Trace event{trace_info, response};
          amend_order_ack(event, user_id, order_id, version);
        };
    (*connection_)("amend_order"sv, request, callback);
  });
}

void OrderEntry::amend_order_ack(
    Trace<web::rest::Response> const &event, uint8_t user_id, uint32_t order_id, uint32_t version) {
  profile_.amend_order_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::AmendOrder amend_order{body, decode_buffer_};
      log::debug("amend_order={}"sv, amend_order);
      Trace event_2{event, amend_order};
      (*this)(event_2, user_id, order_id, version);
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      auto response = oms::Response{
          .type = RequestType::MODIFY_ORDER,
          .origin = origin,
          .status = status,
          .error = error,
          .text = text,
          .version = version,
          .request_id = {},
          .quantity = NaN,
          .price = NaN,
      };
      Trace event_2{event, response};
      (*this)(event_2, user_id, order_id);
    };
    process_response(event, handle_success, handle_error);
  });
}

// XXX this is a little weird -- the response tells us the last known (?) status of the order
void OrderEntry::operator()(
    Trace<json::AmendOrder> const &event, uint8_t user_id, uint32_t order_id, uint32_t version) {
  auto &[trace_info, amend_order] = event;
  log::info<2>("amend_order={}"sv, amend_order);
  auto status = amend_order.ret_code == 0 ? RequestStatus::ACCEPTED : RequestStatus::REJECTED;
  auto error = json::map_error(amend_order.ret_code);
  auto text = amend_order.ret_msg;
  auto response = oms::Response{
      .type = RequestType::MODIFY_ORDER,
      .origin = Origin::EXCHANGE,
      .status = status,
      .error = error,
      .text = text,
      .version = version,
      .request_id = {},
      .quantity = NaN,
      .price = NaN,
  };
  auto &result = amend_order.result;
  auto side = json::map(result.side);
  auto order_type = json::map(result.order_type);
  auto time_in_force = json::map(result.time_in_force);
  auto order_status = json::map(result.status);
  auto remaining_quantity = result.order_qty - result.exec_qty;
  auto order_update = oms::OrderUpdate{
      .account = account_.get_name(),
      .exchange = Flags::exchange(),
      .symbol = result.symbol,
      .side = side,
      .position_effect = {},
      .max_show_quantity = NaN,
      .order_type = order_type,
      .time_in_force = time_in_force,
      .execution_instructions = {},
      .create_time_utc = {},
      .update_time_utc = {},
      .external_account = {},
      .external_order_id = result.order_id,
      .status = order_status,
      .quantity = result.order_qty,
      .price = result.order_price,
      .stop_price = NaN,
      .remaining_quantity = remaining_quantity,
      .traded_quantity = result.exec_qty,
      .average_traded_price = NaN,
      .last_traded_quantity = NaN,
      .last_traded_price = NaN,
      .last_liquidity = {},
      .update_type = UpdateType::INCREMENTAL,
      .sending_time_utc = amend_order.time,
  };
  Trace event_2{trace_info, response};
  (*this)(event_2, user_id, order_id, order_update);
}

void OrderEntry::cancel_order(
    Event<CancelOrder> const &event,
    oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  profile_.cancel_order([&]() {
    if (!ready())
      throw oms::NotReady{"not ready"sv};
    auto &[message_info, cancel_order] = event;
    auto const path = "/v5/order/cancel"sv;
    std::string buffer;  // XXX
    auto body = json::cancel_order(buffer, cancel_order, order, request_id, previous_request_id, shared_.category);
    log::debug(R"(body="{}")"sv, body);
    auto headers = account_.create_headers(path, {}, body);
    auto request = web::rest::Request{
        .method = web::http::Method::POST,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = web::http::ContentType::APPLICATION_JSON,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
    };
    auto callback =
        [this, user_id = message_info.source, order_id = cancel_order.order_id, version = cancel_order.version](
            [[maybe_unused]] auto &request_id, auto &response) {
          TraceInfo trace_info;
          Trace event{trace_info, response};
          cancel_order_ack(event, user_id, order_id, version);
        };
    (*connection_)("cancel_order"sv, request, callback);
  });
}

void OrderEntry::cancel_order_ack(
    Trace<web::rest::Response> const &event, uint8_t user_id, uint32_t order_id, uint32_t version) {
  profile_.cancel_order_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::CancelOrder cancel_order{body, decode_buffer_};
      log::debug("cancel_order={}"sv, cancel_order);
      Trace event_2{event, cancel_order};
      (*this)(event_2, user_id, order_id, version);
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      auto response = oms::Response{
          .type = RequestType::CANCEL_ORDER,
          .origin = origin,
          .status = status,
          .error = error,
          .text = text,
          .version = version,
          .request_id = {},
          .quantity = NaN,
          .price = NaN,
      };
      Trace event_2{event, response};
      (*this)(event_2, user_id, order_id);
    };
    process_response(event, handle_success, handle_error);
  });
}

// XXX this is a little weird -- the response tells us the last known (?) status of the order
void OrderEntry::operator()(
    Trace<json::CancelOrder> const &event, uint8_t user_id, uint32_t order_id, uint32_t version) {
  auto &[trace_info, cancel_order] = event;
  log::info<2>("cancel_order={}"sv, cancel_order);
  auto status = cancel_order.ret_code == 0 ? RequestStatus::ACCEPTED : RequestStatus::REJECTED;
  auto error = json::map_error(cancel_order.ret_code);
  auto text = cancel_order.ret_msg;
  auto response = oms::Response{
      .type = RequestType::CANCEL_ORDER,
      .origin = Origin::EXCHANGE,
      .status = status,
      .error = error,
      .text = text,
      .version = version,
      .request_id = {},
      .quantity = NaN,
      .price = NaN,
  };
  auto &result = cancel_order.result;
  auto side = json::map(result.side);
  auto order_type = json::map(result.order_type);
  auto time_in_force = json::map(result.time_in_force);
  auto order_status = json::map(result.status);
  auto remaining_quantity = result.order_qty - result.exec_qty;
  auto order_update = oms::OrderUpdate{
      .account = account_.get_name(),
      .exchange = Flags::exchange(),
      .symbol = result.symbol,
      .side = side,
      .position_effect = {},
      .max_show_quantity = NaN,
      .order_type = order_type,
      .time_in_force = time_in_force,
      .execution_instructions = {},
      .create_time_utc = {},
      .update_time_utc = utils::safe_cast(result.cancel_time),
      .external_account = {},
      .external_order_id = result.order_id,
      .status = order_status,
      .quantity = result.order_qty,
      .price = result.order_price,
      .stop_price = NaN,
      .remaining_quantity = remaining_quantity,
      .traded_quantity = result.exec_qty,
      .average_traded_price = NaN,
      .last_traded_quantity = NaN,
      .last_traded_price = NaN,
      .last_liquidity = {},
      .update_type = UpdateType::INCREMENTAL,
      .sending_time_utc = cancel_order.time,
  };
  Trace event_2{trace_info, response};
  (*this)(event_2, user_id, order_id, order_update);
}

void OrderEntry::cancel_all_orders(
    Event<CancelAllOrders> const &event, [[maybe_unused]] std::string_view const &request_id) {
  profile_.cancel_all_orders([&]() {
    if (!ready()) {
      log::warn("*** NOT POSSIBLE TO CANCEL ALL OPEN ORDERS (NOT READY) ***"sv);
      return;
    }
    auto &[message_info, cancel_all_orders] = event;
    auto const path = "/v5/order/cancel-all"sv;
    std::string buffer;  // XXX
    if (shared_.dispatcher.get_all_order_symbols(
            [&](auto &symbol) {
              auto body = json::cancel_all_orders(buffer, cancel_all_orders, request_id, symbol, shared_.category);
              log::debug(R"(body="{}")"sv, body);
              auto headers = account_.create_headers(path, {}, body);
              auto request = web::rest::Request{
                  .method = web::http::Method::POST,
                  .path = path,
                  .query = {},
                  .accept = web::http::Accept::APPLICATION_JSON,
                  .content_type = web::http::ContentType::APPLICATION_JSON,
                  .headers = headers,
                  .body = body,
                  .quality_of_service = {},
              };
              auto callback = [this]([[maybe_unused]] auto &request_id, auto &response) {
                TraceInfo trace_info;
                Trace event{trace_info, response};
                cancel_all_orders_ack(event);
              };
              (*connection_)("cancel_all_orders"sv, request, callback);
            },
            account_.get_name())) {
    } else {
      log::warn("*** NOT POSSIBLE TO CANCEL ALL OPEN ORDERS (NO SYMBOLS) ***"sv);
    }
  });
}

void OrderEntry::cancel_all_orders_ack(Trace<web::rest::Response> const &event) {
  profile_.cancel_all_orders_ack([&]() {
    auto handle_success = [&](auto &body) {
      json::CancelAllOrders cancel_all_orders{body, decode_buffer_};
      log::debug("cancel_all_orders={}"sv, cancel_all_orders);
      Trace event_2{event, cancel_all_orders};
      (*this)(event_2);
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<json::CancelAllOrders> const &event) {
  auto &[trace_info, cancel_all_orders] = event;
  log::info<2>("cancel_all_orders={}"sv, cancel_all_orders);
}

template <typename SuccessHandler, typename ErrorHandler>
void OrderEntry::process_response(
    web::rest::Response const &response, SuccessHandler success_handler, ErrorHandler error_handler) {
  try {
    auto [status, category, body] = response.result();
    log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
    switch (category) {
      using enum web::http::Category;
      case SUCCESS:  // 2xx
        success_handler(body);
        break;
      case CLIENT_ERROR:  // 4xx
        switch (status) {
          using enum web::http::Status;
          case FORBIDDEN:           // 403
            waf_limit_violation();  // note! this is *very* serious
            [[fallthrough]];
          case I_AM_A_TEAPOT:        // 418
          case TOO_MANY_REQUESTS: {  // 429
            auto text = fmt::format("{}"sv, status);
            error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::REQUEST_RATE_LIMIT_REACHED, text);
            break;
          }
          case CONFLICT:  // 409
            assert(false);
            [[fallthrough]];
          default: {
            // json::Error error{body};
            // error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, json::guess_error(error.code), error.msg);
          }
        }
        break;
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

template <typename... Args>
void OrderEntry::operator()(Trace<oms::Response> const &event, uint8_t user_id, uint32_t order_id, Args &&...args) {
  auto &[trace_info, response] = event;
  if (shared_.update_order(
          user_id,
          order_id,
          stream_id_,
          trace_info,
          response,
          std::forward<Args>(args)...,
          []([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("Did not find order: user_id={}, order_id={}"sv, user_id, order_id);
  }
}

template <typename... Args>
void OrderEntry::operator()(
    Trace<oms::OrderUpdate> const &event, std::string_view const &client_order_id, Args &&...args) {
  auto &[trace_info, order_update] = event;
  if (shared_.update_order(
          client_order_id, stream_id_, trace_info, order_update, [&]([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("*** EXTERNAL ORDER ***"sv);
  }
}

void OrderEntry::waf_limit_violation() {
  if (Flags::rest_terminate_on_403()) {
    log::fatal("WAF limit violation"sv);
  } else {
    log::warn("WAF limit violation"sv);
    (*connection_).suspend(Flags::rest_back_off_delay());
  }
}

}  // namespace bybit
}  // namespace roq
