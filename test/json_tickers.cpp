/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/bybit/json/parser.hpp"
#include "roq/bybit/json/tickers.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const SPOT = R"({)"
                  R"("topic":"tickers.BTCUSDT",)"
                  R"("ts":1682083233096,)"
                  R"("type":"snapshot",)"
                  R"("cs":792472763,)"
                  R"("data":{)"
                  R"("symbol":"BTCUSDT",)"
                  R"("lastPrice":"27579.99",)"
                  R"("highPrice24h":"35000",)"
                  R"("lowPrice24h":"26900",)"
                  R"("prevPrice24h":"29005.97",)"
                  R"("volume24h":"240.62801",)"
                  R"("turnover24h":"6905439.32558533",)"
                  R"("price24hPcnt":"-0.0492",)"
                  R"("usdIndexPrice":"28190.58008011")"
                  R"(})"
                  R"(})"sv;

auto const LINEAR = R"({)"
                    R"("topic":"tickers.BTCUSDT",)"
                    R"("type":"snapshot",)"
                    R"("data":{)"
                    R"("symbol":"BTCUSDT",)"
                    R"("tickDirection":"MinusTick",)"
                    R"("price24hPcnt":"-0.005724",)"
                    R"("lastPrice":"27339.40",)"
                    R"("prevPrice24h":"27496.80",)"
                    R"("highPrice24h":"31666.60",)"
                    R"("lowPrice24h":"22800.00",)"
                    R"("prevPrice1h":"27278.20",)"
                    R"("markPrice":"27342.30",)"
                    R"("indexPrice":"27349.74",)"
                    R"("openInterest":"92578.634",)"
                    R"("openInterestValue":"2531312784.42",)"
                    R"("turnover24h":"3391168773.5801",)"
                    R"("volume24h":"123331.4080",)"
                    R"("nextFundingTime":"1682179200000",)"
                    R"("fundingRate":"0.0001",)"
                    R"("bid1Price":"27300.00",)"
                    R"("bid1Size":"1.305",)"
                    R"("ask1Price":"27339.50",)"
                    R"("ask1Size":"22.439")"
                    R"(},)"
                    R"("cs":8065194775,)"
                    R"("ts":1682169027329)"
                    R"(})"sv;

auto const INVERSE = R"({)"
                     R"("topic":"tickers.BTCUSDM23",)"
                     R"("type":"snapshot",)"
                     R"("data":{)"
                     R"("symbol":"BTCUSDM23",)"
                     R"("tickDirection":"MinusTick",)"
                     R"("price24hPcnt":"-0.024403",)"
                     R"("lastPrice":"27625.00",)"
                     R"("prevPrice24h":"28316.00",)"
                     R"("highPrice24h":"28429.50",)"
                     R"("lowPrice24h":"27375.50",)"
                     R"("prevPrice1h":"27552.50",)"
                     R"("markPrice":"27603.76",)"
                     R"("indexPrice":"27309.94",)"
                     R"("openInterest":"2376127",)"
                     R"("openInterestValue":"86.08",)"
                     R"("turnover24h":"76.3171",)"
                     R"("volume24h":"2110680",)"
                     R"("deliveryTime":"2023-06-30T08:00:00Z",)"
                     R"("basisRate":"0.01066315",)"
                     R"("deliveryFeeRate":"0.0005",)"
                     R"("predictedDeliveryPrice":"0.00",)"
                     R"("basis":"315.06",)"
                     R"("nextFundingTime":"",)"
                     R"("fundingRate":"",)"
                     R"("bid1Price":"27594.50",)"
                     R"("bid1Size":"8298",)"
                     R"("ask1Price":"27606.00",)"
                     R"("ask1Size":"9369")"
                     R"(},)"
                     R"("cs":12582755307,)"
                     R"("ts":1682169888078)"
                     R"(})"sv;

auto const OPTION = R"({)"
                    R"("id":"tickers.BTC-28APR23-30000-C-1267284327-1682247012340",)"
                    R"("topic":"tickers.BTC-28APR23-30000-C",)"
                    R"("ts":1682247012340,)"
                    R"("data":{)"
                    R"("symbol":"BTC-28APR23-30000-C",)"
                    R"("bidPrice":"0",)"
                    R"("bidSize":"0",)"
                    R"("bidIv":"0",)"
                    R"("askPrice":"0",)"
                    R"("askSize":"0",)"
                    R"("askIv":"0",)"
                    R"("lastPrice":"40",)"
                    R"("highPrice24h":"40",)"
                    R"("lowPrice24h":"40",)"
                    R"("markPrice":"35.85848319",)"
                    R"("indexPrice":"27543.41",)"
                    R"("markPriceIv":"0.4617",)"
                    R"("underlyingPrice":"27546.96",)"
                    R"("openInterest":"22.01",)"
                    R"("turnover24h":"276.3255",)"
                    R"("volume24h":"0.01",)"
                    R"("totalVolume":"23",)"
                    R"("totalTurnover":"644804",)"
                    R"("delta":"0.058091",)"
                    R"("gamma":"0.00007897",)"
                    R"("vega":"3.69996245",)"
                    R"("theta":"-17.49538621",)"
                    R"("predictedDeliveryPrice":"0",)"
                    R"("change24h":"-0.92307693"},)"
                    R"("type":"snapshot")"
                    R"(})"sv;
}  // namespace

// spot

TEST_CASE("json_tickers_simple_spot", "[json_tickers]") {
  core::Buffer buffer(8192);
  json::Tickers ticks{SPOT, buffer};
}

TEST_CASE("json_tickers_parser_spot", "[json_tickers]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
    void operator()(Trace<json::Tickers> const &) override { found = true; }
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
  auto res = json::Parser::dispatch(handler, SPOT, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}

// linear

TEST_CASE("json_tickers_simple_linear", "[json_tickers]") {
  core::Buffer buffer(8192);
  json::Tickers ticks{LINEAR, buffer};
}

TEST_CASE("json_tickers_parser_linear", "[json_tickers]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
    void operator()(Trace<json::Tickers> const &) override { found = true; }
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
  auto res = json::Parser::dispatch(handler, LINEAR, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}

// inverse

TEST_CASE("json_tickers_simple_inverse", "[json_tickers]") {
  core::Buffer buffer(8192);
  json::Tickers ticks{INVERSE, buffer};
}

TEST_CASE("json_tickers_parser_inverse", "[json_tickers]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
    void operator()(Trace<json::Tickers> const &) override { found = true; }
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
  auto res = json::Parser::dispatch(handler, INVERSE, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}

// option

TEST_CASE("json_tickers_simple_option", "[json_tickers]") {
  core::Buffer buffer(8192);
  json::Tickers ticks{OPTION, buffer};
}

TEST_CASE("json_tickers_parser_option", "[json_tickers]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
    void operator()(Trace<json::Tickers> const &) override { found = true; }
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
  auto res = json::Parser::dispatch(handler, OPTION, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}
