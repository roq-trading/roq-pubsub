/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/io/web/uri.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/server.hpp"

#include "roq/bybit/shared.hpp"

#include "roq/bybit/json/category.hpp"
#include "roq/bybit/json/parser.hpp"

namespace roq {
namespace bybit {

struct MarketData final : public web::socket::Client::Handler, public json::Parser::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<MarketStatus> const &, bool is_last) = 0;
    virtual void operator()(Trace<TopOfBook> const &, bool is_last) = 0;
    virtual void operator()(Trace<MarketByPriceUpdate> const &, bool is_last) = 0;
    virtual void operator()(Trace<TradeSummary> const &, bool is_last) = 0;
    virtual void operator()(Trace<StatisticsUpdate> const &, bool is_last) = 0;
  };

  MarketData(Handler &, io::Context &, uint16_t stream_id, Shared &, size_t index);

  MarketData(MarketData &&) = delete;
  MarketData(MarketData const &) = delete;

  uint16_t stream_id() const { return stream_id_; }

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

  void subscribe(size_t start_from = 0);

 protected:
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

 private:
  void operator()(ConnectionStatus);

  void subscribe(std::span<Symbol const> const &symbols);
  void subscribe(std::string_view const &topic, std::span<Symbol const> const &symbols);

  void send_ping(std::chrono::nanoseconds now);

  void parse(std::string_view const &message);

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
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  size_t const index_;
  std::chrono::nanoseconds const ping_frequency_;
  bool const spot_;
  size_t const mbp_depth_;
  std::string const mbp_topic_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::Buffer decode_buffer_;
  // session
  uint64_t request_id_ = {};
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, order_book, trade, tickers;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
  } latency_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  // ping
  std::chrono::nanoseconds next_ping_ = {};
};

}  // namespace bybit
}  // namespace roq
