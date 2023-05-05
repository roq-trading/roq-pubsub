/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/bybit/json/parser.hpp"
#include "roq/bybit/json/public_trade.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const MESSAGE = R"({)"
                     R"("topic":"publicTrade.BTCUSDT",)"
                     R"("ts":1682084122523,)"
                     R"("type":"snapshot",)"
                     R"("data":[{)"
                     R"("i":"2100000000019296063",)"
                     R"("T":1682084122521,)"
                     R"("p":"27200",)"
                     R"("v":"0.000221",)"
                     R"("S":"Buy",)"
                     R"("s":"BTCUSDT",)"
                     R"("BT":false)"
                     R"(})"
                     R"(])"
                     R"(})"sv;
}  // namespace

TEST_CASE("json_public_trade_simple", "[json_public_trade]") {
  core::Buffer buffer(8192);
  json::PublicTrade trade{MESSAGE, buffer};
}

TEST_CASE("json_public_trade_parser", "[json_public_trade]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { found = true; }
    void operator()(Trace<json::Tickers> const &) override { FAIL(); }
    // private
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Wallet> const &) override { FAIL(); }
    void operator()(Trace<json::Position> const &) override { FAIL(); }
    void operator()(Trace<json::Order> const &) override { FAIL(); }
    void operator()(Trace<json::Execution2> const &) override { FAIL(); }

    bool found = false;
  } handler;
  core::Buffer buffer(8192);
  core::json::Buffer buffer_2{buffer};
  auto res = json::Parser::dispatch(handler, MESSAGE, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}
