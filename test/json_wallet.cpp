/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/parser.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

// note! from websocket

namespace {
auto const MESSAGE = R"({)"
                     R"("id":"460579-2-c4815897-f283-4ccd-b4d7-dac6bde51502-1682339173119",)"
                     R"("topic":"wallet",)"
                     R"("creationTime":1682339173118,)"
                     R"("data":[{)"
                     R"("accountType":"SPOT",)"
                     R"("accountIMRate":"",)"
                     R"("accountMMRate":"",)"
                     R"("accountLTV":"",)"
                     R"("totalEquity":"",)"
                     R"("totalWalletBalance":"",)"
                     R"("totalMarginBalance":"",)"
                     R"("totalAvailableBalance":"",)"
                     R"("totalPerpUPL":"",)"
                     R"("totalInitialMargin":"",)"
                     R"("totalMaintenanceMargin":"",)"
                     R"("coin":[{)"
                     R"("coin":"USDT",)"
                     R"("equity":"",)"
                     R"("usdValue":"",)"
                     R"("walletBalance":"48855.56",)"
                     R"("free":"48855.56",)"
                     R"("locked":"0",)"
                     R"("availableToWithdraw":"",)"
                     R"("availableToBorrow":"",)"
                     R"("borrowAmount":"",)"
                     R"("accruedInterest":"",)"
                     R"("totalOrderIM":"",)"
                     R"("totalPositionIM":"",)"
                     R"("totalPositionMM":"",)"
                     R"("unrealisedPnl":"",)"
                     R"("cumRealisedPnl":"")"
                     R"(})"
                     R"(])"
                     R"(})"
                     R"(])"
                     R"(})"sv;

auto const MESSAGE_2 = R"({)"
                       R"("id":"460579-3-a5da51e6-2a7e-4dc6-9546-d85721fb0d1b-1682341631264",)"
                       R"("topic":"wallet",)"
                       R"("creationTime":1682341631264,)"
                       R"("data":[{)"
                       R"("accountType":"SPOT",)"
                       R"("accountIMRate":"",)"
                       R"("accountMMRate":"",)"
                       R"("accountLTV":"",)"
                       R"("totalEquity":"",)"
                       R"("totalWalletBalance":"",)"
                       R"("totalMarginBalance":"",)"
                       R"("totalAvailableBalance":"",)"
                       R"("totalPerpUPL":"",)"
                       R"("totalInitialMargin":"",)"
                       R"("totalMaintenanceMargin":"",)"
                       R"("coin":[{)"
                       R"("coin":"USDT",)"
                       R"("equity":"",)"
                       R"("usdValue":"",)"
                       R"("walletBalance":"48855.56",)"
                       R"("free":"48828.5061",)"
                       R"("locked":"27.0539",)"
                       R"("availableToWithdraw":"",)"
                       R"("availableToBorrow":"",)"
                       R"("borrowAmount":"",)"
                       R"("accruedInterest":"",)"
                       R"("totalOrderIM":"",)"
                       R"("totalPositionIM":"",)"
                       R"("totalPositionMM":"",)"
                       R"("unrealisedPnl":"",)"
                       R"("cumRealisedPnl":"")"
                       R"(})"
                       R"(])"
                       R"(})"
                       R"(])"
                       R"(})";
}  // namespace

TEST_CASE("json_wallet_parser", "[json_wallet]") {
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
    void operator()(Trace<json::Wallet> const &event) override {
      found = true;
      auto &[trace_info, wallet_balance] = event;
      CHECK(wallet_balance.account_type == json::AccountType::SPOT);
      REQUIRE(std::size(wallet_balance.coin) == 1);
      auto &c0 = wallet_balance.coin[0];
      CHECK(c0.coin == "USDT"sv);
    }
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

TEST_CASE("json_wallet_parser_2", "[json_wallet]") {
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
    void operator()(Trace<json::Wallet> const &event) override {
      found = true;
      auto &[trace_info, wallet_balance] = event;
      CHECK(wallet_balance.account_type == json::AccountType::SPOT);
      REQUIRE(std::size(wallet_balance.coin) == 1);
      auto &c0 = wallet_balance.coin[0];
      CHECK(c0.coin == "USDT"sv);
    }
    void operator()(Trace<json::Position> const &) override { FAIL(); }
    void operator()(Trace<json::Order> const &) override { FAIL(); }
    void operator()(Trace<json::Execution2> const &) override { FAIL(); }

    bool found = false;
  } handler;
  core::Buffer buffer(8192);
  core::json::Buffer buffer_2{buffer};
  auto res = json::Parser::dispatch(handler, MESSAGE_2, buffer_2, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}
