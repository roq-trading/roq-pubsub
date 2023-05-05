/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/amend_order.hpp"
#include "roq/bybit/json/utils.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto SYMBOL = "BTCUSDT"sv;
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

TEST_CASE("json_amend_order_price", "[json_amend_order]") {
  std::string buffer;
  auto modify_order = ModifyOrder{
      .account = "A1"sv,
      .order_id = 1000,
      .request_template = {},
      .quantity = NaN,
      .price = 23456.78,
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
  };
  auto order = create_order();
  auto request_id = "2345"sv;
  auto previous_request_id = "1234"sv;
  json::amend_order(buffer, modify_order, order, request_id, previous_request_id, json::Category::SPOT);
  auto expected = R"({)"
                  R"("category":"spot",)"
                  R"("symbol":"BTCUSDT",)"
                  R"("price":"23456.78",)"
                  R"("orderLinkId":"1234")"
                  R"(})";
  CHECK(buffer == expected);
}

TEST_CASE("json_amend_order_quantity", "[json_amend_order]") {
  std::string buffer;
  auto modify_order = ModifyOrder{
      .account = "A1"sv,
      .order_id = 1000,
      .request_template = {},
      .quantity = 1.2345,
      .price = NaN,
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
  };
  auto order = create_order();
  auto request_id = "2345"sv;
  auto previous_request_id = "1234"sv;
  json::amend_order(buffer, modify_order, order, request_id, previous_request_id, json::Category::SPOT);
  auto expected = R"({)"
                  R"("category":"spot",)"
                  R"("symbol":"BTCUSDT",)"
                  R"("qty":"1.2345",)"
                  R"("orderLinkId":"1234")"
                  R"(})";
  CHECK(buffer == expected);
}

TEST_CASE("json_amend_order_both", "[json_amend_order]") {
  std::string buffer;
  auto modify_order = ModifyOrder{
      .account = "A1"sv,
      .order_id = 1000,
      .request_template = {},
      .quantity = 1.2345,
      .price = 23456.78,
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
  };
  auto order = create_order();
  auto request_id = "2345"sv;
  auto previous_request_id = "1234"sv;
  json::amend_order(buffer, modify_order, order, request_id, previous_request_id, json::Category::SPOT);
  auto expected = R"({)"
                  R"("category":"spot",)"
                  R"("symbol":"BTCUSDT",)"
                  R"("price":"23456.78",)"
                  R"("qty":"1.2345",)"
                  R"("orderLinkId":"1234")"
                  R"(})";
  CHECK(buffer == expected);
}

namespace {
auto const ERROR = R"({)"
                   R"("retCode":10001,)"
                   R"("retMsg":"Illegal category",)"
                   R"("result":{},)"
                   R"("retExtInfo":{},)"
                   R"("time":1682911123716)"
                   R"(})";
}

TEST_CASE("json_amend_order_error", "[json_amend_order]") {
  core::Buffer buffer(8192);
  json::AmendOrder obj{ERROR, buffer};
}
