/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/bybit/json/error.hpp"
#include "roq/bybit/json/parser.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const MESSAGE = R"({)"
                     R"("retCode":10004,)"
                     R"("retMsg":"error sign! origin_string[1682321990138dBW7d0xxu8cbXUl5vq5000category=linear]",)"
                     R"("result":{},)"
                     R"("retExtInfo":{},)"
                     R"("time":1682321990235)"
                     R"(})"sv;
}  // namespace

TEST_CASE("json_error_simple", "[json_error]") {
  core::Buffer buffer(8192);
  json::Error obj{MESSAGE, buffer};
}

/*
TEST_CASE("json_error_parser", "[json_error]") {
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Error> const &) override { found = true; }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Subscribe> const &) override { FAIL(); }
    // public
    void operator()(Trace<json::OrderBook> const &, [[maybe_unused]] size_t depth) override { FAIL(); }
    void operator()(Trace<json::PublicTrade> const &) override { FAIL(); }
    void operator()(Trace<json::Tickers> const &) override { FAIL(); }
    // private
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Wallet> const &) override { FAIL(); }
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
*/
