/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/buffer.hpp"
#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/server.hpp"

#include "roq/bybit/account.hpp"
#include "roq/bybit/order_entry_state.hpp"
#include "roq/bybit/shared.hpp"

#include "roq/bybit/json/account_info.hpp"
#include "roq/bybit/json/execution.hpp"
#include "roq/bybit/json/open_orders.hpp"
#include "roq/bybit/json/position_info.hpp"

#include "roq/bybit/json/wallet_parser.hpp"

#include "roq/bybit/json/amend_order.hpp"
#include "roq/bybit/json/cancel_all_orders.hpp"
#include "roq/bybit/json/cancel_order.hpp"
#include "roq/bybit/json/place_order.hpp"

namespace roq {
namespace bybit {

struct OrderEntry final : public web::rest::Client::Handler, public json::WalletParser::Handler {
  struct Response final {
    std::string_view account;
    std::string_view topic;
    std::string_view symbol;
  };
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<oms::TradeUpdate> const &, uint16_t stream_id, bool is_last, uint8_t user_id) = 0;
    virtual void operator()(Trace<PositionUpdate> const &, bool is_last) = 0;
    virtual void operator()(Trace<FundsUpdate> const &, bool is_last) = 0;
    //
    virtual void operator()(Trace<Response> const &) = 0;
  };

  OrderEntry(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  OrderEntry(OrderEntry &&) = delete;
  OrderEntry(OrderEntry const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

  uint16_t operator()(Event<CreateOrder> const &, oms::Order const &, std::string_view const &request_id);
  uint16_t operator()(
      Event<ModifyOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  uint16_t operator()(
      Event<CancelOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id);

 protected:
  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;
  void operator()(Trace<web::rest::Response> const &, uint64_t request_id, uint64_t opaque) override;

  void operator()(ConnectionStatus);

  uint32_t download(OrderEntryState state);

  void check_request_queue(std::chrono::nanoseconds now);

  void get_account_info();
  void get_account_info_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::AccountInfo> const &);

  void get_wallet_balance();
  void get_wallet_balance_ack(Trace<web::rest::Response> const &);
  void operator()(Trace<json::Wallet> const &) override;

  void get_position_info(std::string_view const &symbol);
  void get_position_info_ack(Trace<web::rest::Response> const &, std::string_view const &symbol);
  void operator()(Trace<json::PositionInfo> const &);

  void get_open_orders(std::string_view const &symbol);
  void get_open_orders_ack(Trace<web::rest::Response> const &, std::string_view const &symbol);
  void operator()(Trace<json::OpenOrders> const &);

  void get_execution(std::string_view const &symbol);
  void get_execution_ack(Trace<web::rest::Response> const &, std::string_view const &symbol);
  void operator()(Trace<json::Execution> const &);

  void place_order(Event<CreateOrder> const &, oms::Order const &, std::string_view const &request_id);
  void place_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint32_t order_id, uint32_t version);
  void operator()(Trace<json::PlaceOrder> const &, uint8_t user_id, uint32_t order_id, uint32_t version);

  void amend_order(
      Event<ModifyOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  void amend_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint32_t order_id, uint32_t version);
  void operator()(Trace<json::AmendOrder> const &, uint8_t user_id, uint32_t order_id, uint32_t version);

  void cancel_order(
      Event<CancelOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  void cancel_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint32_t order_id, uint32_t version);
  void operator()(Trace<json::CancelOrder> const &, uint8_t user_id, uint32_t order_id, uint32_t version);

  void cancel_all_orders(Event<CancelAllOrders> const &, std::string_view const &request_id);
  void cancel_all_orders_ack(Trace<web::rest::Response> const &);
  void operator()(Trace<json::CancelAllOrders> const &);

  template <typename SuccessHandler, typename ErrorHandler>
  void process_response(web::rest::Response const &, SuccessHandler, ErrorHandler);

  template <typename... Args>
  void operator()(Trace<oms::Response> const &, uint8_t user_id, uint32_t order_id, Args &&...);

  template <typename... Args>
  void operator()(Trace<oms::OrderUpdate> const &, std::string_view const &client_order_id, Args &&...);

  void waf_limit_violation();

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // connection
  std::unique_ptr<web::rest::Client> const connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile account_info, account_info_ack,  //
        wallet_balance, wallet_balance_ack,                 //
        position_info, position_info_ack,                   //
        open_orders, open_orders_ack,                       //
        execution, execution_ack,                           //
        place_order, place_order_ack,                       //
        amend_order, amend_order_ack,                       //
        cancel_order, cancel_order_ack,                     //
        cancel_all_orders, cancel_all_orders_ack;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  core::Download<OrderEntryState> download_;
};

}  // namespace bybit
}  // namespace roq
