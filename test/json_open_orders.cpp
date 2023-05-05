/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/open_orders.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const EMPTY = R"({)"
                   R"("retCode":0,)"
                   R"("retMsg":"OK",)"
                   R"("result":{)"
                   R"("list":[],)"
                   R"("nextPageCursor":"",)"
                   R"("category":"linear")"
                   R"(},)"
                   R"("retExtInfo":{},)"
                   R"("time":1682689733889)"
                   R"(})"sv;
}  // namespace

TEST_CASE("json_open_orders_empty", "[json_open_orders]") {
  core::Buffer buffer(8192);
  json::OpenOrders obj{EMPTY, buffer};
}

namespace {
auto const MESSAGE_SPOT = R"({)"
                          R"("retCode":0,)"
                          R"("retMsg":"OK",)"
                          R"("result":{)"
                          R"("nextPageCursor":"1410314510738196224",)"
                          R"("category":"spot",)"
                          R"("list":[{)"
                          R"("orderLinkId":"iQAC6QMAAQAAz1OYoMFC",)"
                          R"("orderId":"1410314510738196224",)"
                          R"("blockTradeId":"",)"
                          R"("symbol":"BTCUSDT",)"
                          R"("price":"28909.99",)"
                          R"("isLeverage":"0",)"
                          R"("positionIdx":0,)"
                          R"("qty":"0.000040",)"
                          R"("side":"Buy",)"
                          R"("orderStatus":"New",)"
                          R"("cancelType":"UNKNOWN",)"
                          R"("rejectReason":"",)"
                          R"("avgPrice":"0",)"
                          R"("leavesQty":"",)"
                          R"("leavesValue":"",)"
                          R"("cumExecQty":"0",)"
                          R"("cumExecValue":"",)"
                          R"("cumExecFee":"",)"
                          R"("timeInForce":"GTC",)"
                          R"("orderType":"Limit",)"
                          R"("stopOrderType":"",)"
                          R"("orderIv":"",)"
                          R"("triggerPrice":"",)"
                          R"("takeProfit":"",)"
                          R"("stopLoss":"",)"
                          R"("tpTriggerBy":"",)"
                          R"("slTriggerBy":"",)"
                          R"("triggerDirection":0,)"
                          R"("triggerBy":"",)"
                          R"("lastPriceOnCreated":"",)"
                          R"("reduceOnly":false,)"
                          R"("closeOnTrigger":false,)"
                          R"("createdTime":"1682858590868",)"
                          R"("updatedTime":"1682858590879",)"
                          R"("smpType":"None",)"
                          R"("smpGroup":0,)"
                          R"("smpOrderId":"")"
                          R"(})"
                          R"(])"
                          R"(})"
                          R"(})"sv;
}  // namespace

TEST_CASE("json_open_orders_spot", "[json_open_orders]") {
  core::Buffer buffer(8192);
  json::OpenOrders obj{MESSAGE_SPOT, buffer};
}

namespace {
auto const MESSAGE_LINEAR = R"({)"
                            R"("retCode":0,)"
                            R"("retMsg":"OK",)"
                            R"("result":{)"
                            R"("list":[{)"
                            R"("orderId":"af6512ea-74b0-4ac5-959a-9a52d4208b08",)"
                            R"("orderLinkId":"VQAC6QMAAQAARFkvRM9C",)"
                            R"("blockTradeId":"",)"
                            R"("symbol":"BTCUSDT",)"
                            R"("price":"27673.40",)"
                            R"("qty":"0.001",)"
                            R"("side":"Buy",)"
                            R"("isLeverage":"",)"
                            R"("positionIdx":0,)"
                            R"("orderStatus":"New",)"
                            R"("cancelType":"UNKNOWN",)"
                            R"("rejectReason":"EC_NoError",)"
                            R"("avgPrice":"0",)"
                            R"("leavesQty":"0.001",)"
                            R"("leavesValue":"27.6734",)"
                            R"("cumExecQty":"0.000",)"
                            R"("cumExecValue":"0",)"
                            R"("cumExecFee":"0",)"
                            R"("timeInForce":"GTC",)"
                            R"("orderType":"Limit",)"
                            R"("stopOrderType":"UNKNOWN",)"
                            R"("orderIv":"",)"
                            R"("triggerPrice":"0.00",)"
                            R"("takeProfit":"0.00",)"
                            R"("stopLoss":"0.00",)"
                            R"("tpTriggerBy":"UNKNOWN",)"
                            R"("slTriggerBy":"UNKNOWN",)"
                            R"("triggerDirection":0,)"
                            R"("triggerBy":"UNKNOWN",)"
                            R"("lastPriceOnCreated":"",)"
                            R"("reduceOnly":false,)"
                            R"("closeOnTrigger":false,)"
                            R"("smpType":"None",)"
                            R"("smpGroup":0,)"
                            R"("smpOrderId":"",)"
                            R"("createdTime":"1682917170023",)"
                            R"("updatedTime":"1682917170026",)"
                            R"("placeType":"")"
                            R"(})"
                            R"(],)"
                            R"("nextPageCursor":"page_args%3Daf6512ea-74b0-4ac5-959a-9a52d4208b08%26",)"
                            R"("category":"linear")"
                            R"(},)"
                            R"("retExtInfo":{},)"
                            R"("time":1682919058550)"
                            R"(})"sv;
}  // namespace

TEST_CASE("json_open_orders_linear", "[json_open_orders]") {
  core::Buffer buffer(8192);
  json::OpenOrders obj{MESSAGE_LINEAR, buffer};
}
