/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/place_order.hpp"
#include "roq/bybit/json/utils.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const SYMBOL = "BTCUSDT"sv;
}

namespace {
auto create_order() {
  auto order = oms::Order{};
  order.symbol = SYMBOL;
  order.price_decimals = Decimals::_2;
  order.quantity_decimals = Decimals::_4;
  return order;
}
}  // namespace

TEST_CASE("json_place_order_simple", "[json_place_order]") {
  std::string buffer;
  auto create_order = CreateOrder{
      .account = "A1"sv,
      .order_id = 1000,
      .exchange = "bybit",
      .symbol = SYMBOL,
      .side = Side::BUY,
      .position_effect = {},
      .max_show_quantity = NaN,
      .order_type = OrderType::LIMIT,
      .time_in_force = TimeInForce::GTC,
      .execution_instructions = {},
      .request_template = {},
      .quantity = 1.2345,
      .price = 23456.78,
      .stop_price = NaN,
      .routing_id = {},
  };
  auto order = ::create_order();
  auto request_id = "1234"sv;
  json::place_order(buffer, create_order, order, request_id, json::Category::SPOT);
  auto expected = R"({)"
                  R"("category":"spot",)"
                  R"("symbol":"BTCUSDT",)"
                  R"("side":"Buy",)"
                  R"("orderType":"Limit",)"
                  R"("qty":"1.2345",)"
                  R"("timeInForce":"GTC",)"
                  R"("reduceOnly":false,)"
                  R"("price":"23456.78",)"
                  R"("orderLinkId":"1234")"
                  R"(})";
  CHECK(buffer == expected);
}

namespace {
auto const MESSAGE = R"({)"
                     R"("retCode":0,)"
                     R"("retMsg":"OK",)"
                     R"("result":{"orderId":"1410305521329702656",)"
                     R"("orderLinkId":"SQAC6QMAAQAASPS4YMFC")"
                     R"(},)"
                     R"("retExtInfo":{},)"
                     R"("time":1682857519260)"
                     R"(})";
}

TEST_CASE("json_place_order_response", "[json_place_order]") {
  core::Buffer buffer(8192);
  json::PlaceOrder obj{MESSAGE, buffer};
}
