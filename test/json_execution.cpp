/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/execution.hpp"
#include "roq/bybit/json/parser.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const MESSAGE = R"({)"
                     R"("retCode":0,)"
                     R"("retMsg":"OK",)"
                     R"("result":{)"
                     R"("list":[{)"
                     R"("symbol":"BTCUSDT",)"
                     R"("orderId":"1682668800-BTCUSDT-460579-Buy",)"
                     R"("orderLinkId":"",)"
                     R"("side":"Buy",)"
                     R"("orderPrice":"0.00",)"
                     R"("orderQty":"0.000",)"
                     R"("leavesQty":"0.000",)"
                     R"("orderType":"UNKNOWN",)"
                     R"("stopOrderType":"UNKNOWN",)"
                     R"("execFee":"0.04486547",)"
                     R"("execId":"7eeb8e40-fde4-4be1-9fa1-191081c49d8e",)"
                     R"("execPrice":"29476.42",)"
                     R"("execQty":"0.002",)"
                     R"("execType":"Funding",)"
                     R"("execValue":"58.95284",)"
                     R"("execTime":"1682668800000",)"
                     R"("isMaker":false,)"
                     R"("feeRate":"0.00076104",)"
                     R"("tradeIv":"",)"
                     R"("markIv":"",)"
                     R"("markPrice":"29476.42",)"
                     R"("indexPrice":"",)"
                     R"("underlyingPrice":"",)"
                     R"("blockTradeId":"",)"
                     R"("closedSize":"0.000")"
                     R"(},{)"
                     R"("symbol":"BTCUSDT",)"
                     R"("orderId":"1682640000-BTCUSDT-460579-Buy",)"
                     R"("orderLinkId":"",)"
                     R"("side":"Buy",)"
                     R"("orderPrice":"0.00",)"
                     R"("orderQty":"0.000",)"
                     R"("leavesQty":"0.000",)"
                     R"("orderType":"UNKNOWN",)"
                     R"("stopOrderType":"UNKNOWN",)"
                     R"("execFee":"-0.06101433",)"
                     R"("execId":"c140901a-2a06-4f3b-bd40-3db229e95c7b",)"
                     R"("execPrice":"29474.10",)"
                     R"("execQty":"0.002",)"
                     R"("execType":"Funding",)"
                     R"("execValue":"58.9482",)"
                     R"("execTime":"1682640000000",)"
                     R"("isMaker":false,)"
                     R"("feeRate":"-0.00103505",)"
                     R"("tradeIv":"",)"
                     R"("markIv":"",)"
                     R"("markPrice":"29474.10",)"
                     R"("indexPrice":"",)"
                     R"("underlyingPrice":"",)"
                     R"("blockTradeId":"",)"
                     R"("closedSize":"0.000")"
                     R"(},{)"
                     R"("symbol":"BTCUSDT",)"
                     R"("orderId":"1682611200-BTCUSDT-460579-Buy",)"
                     R"("orderLinkId":"",)"
                     R"("side":"Buy",)"
                     R"("orderPrice":"0.00",)"
                     R"("orderQty":"0.000",)"
                     R"("leavesQty":"0.000",)"
                     R"("orderType":"UNKNOWN",)"
                     R"("stopOrderType":"UNKNOWN",)"
                     R"("execFee":"0.05848746",)"
                     R"("execId":"dfad755a-6776-44a4-a654-21f3f0687e2d",)"
                     R"("execPrice":"29133.60",)"
                     R"("execQty":"0.002",)"
                     R"("execType":"Funding",)"
                     R"("execValue":"58.2672",)"
                     R"("execTime":"1682611200000",)"
                     R"("isMaker":false,)"
                     R"("feeRate":"0.00100378",)"
                     R"("tradeIv":"",)"
                     R"("markIv":"",)"
                     R"("markPrice":"29133.60",)"
                     R"("indexPrice":"",)"
                     R"("underlyingPrice":"",)"
                     R"("blockTradeId":"",)"
                     R"("closedSize":"0.000")"
                     R"(})"
                     R"(],)"
                     R"("nextPageCursor":"",)"
                     R"("category":"linear")"
                     R"(},)"
                     R"("retExtInfo":{},)"
                     R"("time":1682694922390)"
                     R"(})"sv;
}  // namespace

TEST_CASE("json_execution_simple", "[json_execution]") {
  core::Buffer buffer(8192);
  json::Execution obj{MESSAGE, buffer};
}

namespace {
auto const MESSAGE_LINEAR = R"({)"
                            R"("topic":"execution",)"
                            R"("id":"6e2ab9ee56f4c6bb3dc788ba004a5054:663a60ba7623dfef:0:01",)"
                            R"("creationTime":1682924344208,)"
                            R"("data":[{)"
                            R"("blockTradeId":"",)"
                            R"("category":"linear",)"
                            R"("execFee":"0.01714062",)"
                            R"("execId":"a3333bcf-4d02-55f1-a9d2-08a5149064f4",)"
                            R"("execPrice":"28567.70",)"
                            R"("execQty":"0.001",)"
                            R"("execTime":"1682924344203",)"
                            R"("execType":"Trade",)"
                            R"("execValue":"28.5677",)"
                            R"("feeRate":"0.0006",)"
                            R"("indexPrice":"0.00",)"
                            R"("isLeverage":"",)"
                            R"("isMaker":false,)"
                            R"("leavesQty":"0",)"
                            R"("markIv":"",)"
                            R"("markPrice":"28576.05",)"
                            R"("orderId":"787dcaf8-84b4-44b7-b57c-71bdf1bd5420",)"
                            R"("orderLinkId":"nQAC6QMAAQAAnUSX79BC",)"
                            R"("orderPrice":"28567.70",)"
                            R"("orderQty":"0.001",)"
                            R"("orderType":"Limit",)"
                            R"("symbol":"BTCUSDT",)"
                            R"("stopOrderType":"UNKNOWN",)"
                            R"("side":"Buy",)"
                            R"("tradeIv":"",)"
                            R"("underlyingPrice":"",)"
                            R"("closedSize":"0")"
                            R"(})"
                            R"(])"
                            R"(})";
}

TEST_CASE("json_execution_parser", "[json_execution]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
    void operator()(Trace<json::Tickers> const &) override { FAIL(); }
    // private
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Wallet> const &) override { FAIL(); }
    void operator()(Trace<json::Position> const &) override { FAIL(); }
    void operator()(Trace<json::Order> const &) override { FAIL(); }
    void operator()(Trace<json::Execution2> const &) override { found = true; }

    bool found = false;
  } handler;
  core::Buffer buffer(8192);
  core::json::Buffer buffer_2{buffer};
  auto res = json::Parser::dispatch(handler, MESSAGE_LINEAR, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}
