/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/cancel_all_orders.hpp"
#include "roq/bybit/json/utils.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

/*
namespace {
auto SYMBOL = "BTCUSDT"sv;
}

namespace {
auto create_order() {
  auto order = oms::Order{};
  order.symbol = SYMBOL;
  return order;
}
}  // namespace

TEST_CASE("json_cancel_order_simple", "[json_cancel_order]") {
  std::string buffer;
  auto cancel_order = CancelOrder{
      .account = "A1"sv,
      .order_id = 1000,
      .request_template = {},
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
  };
  auto order = create_order();
  auto request_id = "2345"sv;
  auto previous_request_id = "1234"sv;
  json::cancel_order(buffer, cancel_order, order, request_id, previous_request_id, json::Category::SPOT);
  auto expected = R"({)"
                  R"("category":"spot",)"
                  R"("symbol":"BTCUSDT",)"
                  R"("orderLinkId":"1234")"
                  R"(})";
  CHECK(buffer == expected);
}
*/

namespace {
auto const MESSAGE = R"({)"
                     R"("retCode":0,)"
                     R"("retMsg":"OK",)"
                     R"("result":{)"
                     R"("success":"1")"
                     R"(},)"
                     R"("retExtInfo":{},)"
                     R"("time":1682861453404)"
                     R"(})";
}

TEST_CASE("json_cancel_all_orders_response", "[json_cancel_all_orders]") {
  core::Buffer buffer(8192);
  json::CancelAllOrders obj{MESSAGE, buffer};
}
