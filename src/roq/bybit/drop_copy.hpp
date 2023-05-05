/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>

#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/server.hpp"

#include "roq/bybit/account.hpp"
#include "roq/bybit/shared.hpp"

#include "roq/bybit/order_entry.hpp"  // response
#include "roq/bybit/rest.hpp"         // symbols

#include "roq/bybit/json/parser.hpp"

namespace roq {
namespace bybit {

struct DropCopy final : public web::socket::Client::Handler, json::Parser::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<oms::TradeUpdate> const &, uint16_t stream_id, bool is_last, uint8_t user_id) = 0;
    virtual void operator()(Trace<PositionUpdate> const &, bool is_last) = 0;
    virtual void operator()(Trace<FundsUpdate> const &, bool is_last) = 0;
  };

  DropCopy(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  DropCopy(DropCopy &&) = delete;
  DropCopy(DropCopy const &) = delete;

  bool ready() const;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

  void operator()(Rest::SymbolsUpdate &);

  void operator()(Trace<OrderEntry::Response> const &);

 protected:
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

  // json::Parser::Handler

  void operator()(Trace<json::Ping> const &) override;
  // response
  void operator()(Trace<json::Auth> const &) override;
  void operator()(Trace<json::Subscribe> const &) override;
  void operator()(Trace<json::Error> const &) override;
  // public stream
  void operator()(Trace<json::OrderBook> const &, size_t depth) override;
  void operator()(Trace<json::PublicTrade> const &) override;
  void operator()(Trace<json::Tickers> const &) override;
  // private stream
  void operator()(Trace<json::Wallet> const &) override;
  void operator()(Trace<json::Position> const &) override;
  void operator()(Trace<json::Order> const &) override;
  void operator()(Trace<json::Execution2> const &) override;

 private:
  void operator()(ConnectionStatus);

  void subscribe();

  void subscribe(std::string_view const &topic);

  void parse(std::string_view const &message);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, auth, wallet, order, execution, position;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
  } latency_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  std::chrono::nanoseconds logon_timeout_ = {};
  std::chrono::nanoseconds next_ping_ = {};
  // ...
  absl::flat_hash_set<std::string> symbols_;
};

}  // namespace bybit
}  // namespace roq
