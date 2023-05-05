/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/position_info.hpp"

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
                     R"("positionIdx":0,)"
                     R"("riskId":1,)"
                     R"("riskLimitValue":"2000000",)"
                     R"("symbol":"BTCUSDT",)"
                     R"("side":"Buy",)"
                     R"("size":"0.002",)"
                     R"("avgPrice":"32685.25",)"
                     R"("positionValue":"65.3705",)"
                     R"("tradeMode":0,)"
                     R"("positionStatus":"Normal",)"
                     R"("autoAddMargin":0,)"
                     R"("adlRankIndicator":2,)"
                     R"("leverage":"10",)"
                     R"("markPrice":"29252.90",)"
                     R"("liqPrice":"0.10",)"
                     R"("bustPrice":"0.10",)"
                     R"("positionMM":"0.000001",)"
                     R"("positionIM":"0.653705",)"
                     R"("tpslMode":"Full",)"
                     R"("takeProfit":"0.00",)"
                     R"("stopLoss":"0.00",)"
                     R"("trailingStop":"0.00",)"
                     R"("unrealisedPnl":"-6.8647",)"
                     R"("cumRealisedPnl":"-3.85552684",)"
                     R"("createdTime":"1644842313438",)"
                     R"("updatedTime":"1682668800070")"
                     R"(})"
                     R"(],)"
                     R"("nextPageCursor":"",)"
                     R"("category":"linear")"
                     R"(},)"
                     R"("retExtInfo":{},)"
                     R"("time":1682687025633)"
                     R"(})"sv;
}  // namespace

TEST_CASE("simple", "[json_position_info]") {
  core::Buffer buffer(8192);
  json::PositionInfo obj{MESSAGE, buffer};
  REQUIRE(std::size(obj.result.list) == 1);
}
