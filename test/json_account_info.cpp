/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/bybit/json/account_info.hpp"

using namespace roq;
using namespace roq::bybit;

using namespace std::literals;
using namespace std::chrono_literals;

namespace {
auto const MESSAGE = R"({)"
                     R"("retCode":0,)"
                     R"("retMsg":"OK",)"
                     R"("result":{)"
                     R"("marginMode":"REGULAR_MARGIN",)"
                     R"("updatedTime":"1644843230000",)"
                     R"("unifiedMarginStatus":1,)"
                     R"("dcpStatus":"OFF",)"
                     R"("timeWindow":10,)"
                     R"("smpGroup":0)"
                     R"(})"
                     R"(})"sv;
}  // namespace

TEST_CASE("json_account_info_spot", "[json_account_info]") {
  core::Buffer buffer(8192);
  json::AccountInfo obj{MESSAGE, buffer};
}
