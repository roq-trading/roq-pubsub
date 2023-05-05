/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/gateway.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

#include "roq/logging.hpp"

#include "roq/clock.hpp"
#include "roq/core/charconv.hpp"
#include "roq/core/utils.hpp"

#include "roq/bybit/flags.hpp"

#include "roq/bybit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {

// === HELPERS ===

namespace {
template <typename R>
R create_accounts(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  for (auto &[_, account] : config.accounts)
    result.try_emplace(account.name, std::make_unique<Account>(config, account.name));
  return result;
}

template <typename R>
R create_order_entry(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  for (auto &[name, account] : accounts)
    result.try_emplace(name, std::make_unique<OrderEntry>(gateway, context, ++stream_id, *account, shared));
  return result;
}

template <typename R>
R create_drop_copy(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  for (auto &[name, account] : accounts)
    result.try_emplace(name, std::make_unique<DropCopy>(gateway, context, ++stream_id, *account, shared));
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Gateway::Gateway(server::Dispatcher &dispatcher, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, accounts_{create_accounts<decltype(accounts_)>(config)}, context_{context},
      shared_{dispatcher}, rest_{*this, context_, ++stream_id_, shared_},
      order_entry_{create_order_entry<decltype(order_entry_)>(*this, context_, stream_id_, accounts_, shared_)},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(*this, context_, stream_id_, accounts_, shared_)} {
}

void Gateway::operator()(Event<Start> const &event) {
  log::info("Starting..."sv);
  assert(std::empty(market_data_));
  dispatch(event);
}

void Gateway::operator()(Event<Stop> const &event) {
  log::info("Stopping..."sv);
  dispatch(event);
}

void Gateway::operator()(Event<Timer> const &event) {
  dispatch(event);
}

void Gateway::operator()(Event<Connected> const &) {
}

void Gateway::operator()(Event<Disconnected> const &event) {
  auto const &[message_info, disconnected] = event;
  log::warn(
      R"(Disconnected: source="{}", order_cancel_policy={})"sv,
      message_info.source_name,
      disconnected.order_cancel_policy);
  switch (disconnected.order_cancel_policy) {
    using enum OrderCancelPolicy;
    case UNDEFINED:
      break;
    case MANAGED_ORDERS:
      log::warn("*** CANCEL MANAGED ORDERS NOT IMPLEMENTED ***"sv);
      break;
    case BY_ACCOUNT:
      log::warn("*** CANCEL ALL ACCOUNT ORDERS ***"sv);
      for (auto &[account, order_entry] : order_entry_) {
        if (dispatcher_.can_user_trade_account(account, message_info.source)) {
          log::warn(R"(- account="{}")"sv, account);
          auto cancel_all_orders = CancelAllOrders{
              .account = account,
          };
          Event event{message_info, cancel_all_orders};
          (*order_entry)(event, {});
        }
      }
      break;
  }
}

void Gateway::operator()(Trace<StreamStatus> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ExternalLatency> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ReferenceData> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketStatus> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<TopOfBook> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketByPriceUpdate> const &event, bool is_last) {
  auto callback = []([[maybe_unused]] auto &market_by_price) {};
  dispatcher_(event, is_last, bids_, asks_, callback);
}

void Gateway::operator()(Trace<TradeSummary> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<StatisticsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<oms::TradeUpdate> const &event, uint16_t stream_id, bool is_last, uint8_t user_id) {
  dispatcher_(event, stream_id, is_last, user_id);
}

void Gateway::operator()(Trace<PositionUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<FundsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Rest::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &item : market_data_)
    (*item).subscribe(start_from);
  for (auto &item : drop_copy_)
    (*item.second)(symbols_update);
}

void Gateway::ensure_symbol_slices(size_t size) {
  while (std::size(market_data_) < size) {
    log::debug("Create market-data (user-stream)"sv);
    auto market_data = std::make_unique<MarketData>(*this, context_, ++stream_id_, shared_, std::size(market_data_));
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*market_data, message_info, start);
    market_data_.emplace_back(std::move(market_data));
  }
}

void Gateway::operator()(Trace<OrderEntry::Response> const &event) {
  auto &[trace_info, response] = event;
  get_drop_copy(response.account)(event);
}

uint16_t Gateway::operator()(
    Event<CreateOrder> const &event, oms::Order const &order, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, order, request_id);
}

uint16_t Gateway::operator()(
    Event<ModifyOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(
    Event<CancelOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, request_id);
}

void Gateway::operator()(metrics::Writer &writer) {
  dispatch(writer);
}

template <typename... Args>
void Gateway::dispatch(Args &&...args) {
  auto helper = [&](auto &target) { target(std::forward<Args>(args)...); };
  helper(rest_);
  for (auto &[_, item] : order_entry_)
    helper(*item);
  for (auto &[_, item] : drop_copy_)
    helper(*item);
  for (auto &item : market_data_)
    helper(*item);
}

OrderEntry &Gateway::get_order_entry(std::string_view const &account) {
  auto iter = order_entry_.find(account);
  if (iter != std::end(order_entry_))
    return *(*iter).second;
  throw RuntimeError{R"(Unknown account="{}")"sv, account};
}

DropCopy &Gateway::get_drop_copy(std::string_view const &account) {
  auto iter = drop_copy_.find(account);
  if (iter != std::end(drop_copy_))
    return *(*iter).second;
  log::fatal(R"(Unknown account="{}")"sv, account);
}

}  // namespace bybit
}  // namespace roq
