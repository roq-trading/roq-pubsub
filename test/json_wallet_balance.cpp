/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/wallet_parser.hpp"

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
                     R"("coin":"BTC",)"
                     R"("equity":"",)"
                     R"("usdValue":"",)"
                     R"("walletBalance":"0.21998",)"
                     R"("free":"0.21998",)"
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
                     R"(},{)"
                     R"("coin":"EOS",)"
                     R"("equity":"",)"
                     R"("usdValue":"",)"
                     R"("walletBalance":"2000",)"
                     R"("free":"2000",)"
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
                     R"(},{)"
                     R"("coin":"ETH",)"
                     R"("equity":"",)"
                     R"("usdValue":"",)"
                     R"("walletBalance":"1",)"
                     R"("free":"1",)"
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
                     R"(},{)"
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
                     R"(},{)"
                     R"("coin":"XRP",)"
                     R"("equity":"",)"
                     R"("usdValue":"",)"
                     R"("walletBalance":"10000",)"
                     R"("free":"10000",)"
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
                     R"(},)"
                     R"("retExtInfo":{},)"
                     R"("time":1682340111050)"
                     R"(})"sv;
}  // namespace

TEST_CASE("json_wallet_balance_spot", "[json_wallet_balance]") {
  struct Handler final : public json::WalletParser::Handler {
    void operator()(Trace<json::Wallet> const &event) override {
      found = true;
      auto &[trace_info, wallet_balance] = event;
      CHECK(wallet_balance.account_type == json::AccountType::SPOT);
      REQUIRE(std::size(wallet_balance.coin) == 5);
      auto &c0 = wallet_balance.coin[0];
      CHECK(c0.coin == "BTC"sv);
    }

    bool found = false;
  } handler;
  core::Buffer buffer(8192);
  auto res = json::WalletParser::dispatch(handler, MESSAGE, buffer, {});
  CHECK(res == true);
  CHECK(handler.found == true);
}
