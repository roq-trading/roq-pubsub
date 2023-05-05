/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/bybit/json/parser.hpp"
#include "roq/bybit/json/subscribe.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const SPOT = R"({)"
                  R"("success":true,)"
                  R"("ret_msg":"subscribe",)"
                  R"("conn_id":"13ac3a8f-52dd-411a-81c1-7644f6a1ef8b",)"
                  R"("req_id":"4000001",)"
                  R"("op":"subscribe")"
                  R"(})"sv;
auto const LINEAR = R"({)"
                    R"("success":true,)"
                    R"("ret_msg":"",)"
                    R"("conn_id":"2ef1dd44-e9e0-4b3a-8852-292afa6ae416",)"
                    R"("req_id":"4000001",)"
                    R"("op":"subscribe")"
                    R"(})"sv;
auto const INVERSE = R"({)"
                     R"("success":true,)"
                     R"("ret_msg":"",)"
                     R"("conn_id":"7d269cfe-ad45-40f0-8fd4-7f42fc58c2c3",)"
                     R"("req_id":"4000001",)"
                     R"("op":"subscribe")"
                     R"(})"sv;
auto const OPTION = R"({)"
                    R"("success":true,)"
                    R"("conn_id":"461834fffe84142f-00000001-000c2ffe-1fd4e17e7fe53c8b-2c14741e",)"
                    R"("data":{)"
                    R"("failTopics":[],)"
                    R"("successTopics":[)"
                    R"("orderbook.25.BTC-21APR23-28500-P",)"
                    R"("orderbook.25.BTC-21APR23-27500-C",)"
                    R"("orderbook.25.BTC-21APR23-31500-C")"
                    R"(])"
                    R"(},)"
                    R"("type":"COMMAND_RESP")"
                    R"(})"sv;
}  // namespace

TEST_CASE("json_subscribe_simple_spot", "[json_subscribe]") {
  core::Buffer buffer(8192);
  json::Subscribe obj{SPOT, buffer};
}

TEST_CASE("json_subscribe_parser_spot", "[json_subscribe]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { found = true; }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
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
  auto res = json::Parser::dispatch(handler, SPOT, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("json_subscribe_simple_linear", "[json_subscribe]") {
  core::Buffer buffer(8192);
  json::Subscribe obj{LINEAR, buffer};
}

TEST_CASE("json_subscribe_parser_linear", "[json_subscribe]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { found = true; }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
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
  auto res = json::Parser::dispatch(handler, LINEAR, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("json_subscribe_simple_inverse", "[json_subscribe]") {
  core::Buffer buffer(8192);
  json::Subscribe obj{INVERSE, buffer};
}

TEST_CASE("json_subscribe_parser_inverse", "[json_subscribe]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { found = true; }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
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
  auto res = json::Parser::dispatch(handler, INVERSE, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("json_subscribe_simple_option", "[json_subscribe]") {
  core::Buffer buffer(8192);
  json::Subscribe obj{OPTION, buffer};
}

TEST_CASE("json_subscribe_parser_option", "[json_subscribe]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { found = true; }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
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
  auto res = json::Parser::dispatch(handler, OPTION, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}
