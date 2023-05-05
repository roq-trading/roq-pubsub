/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/shared.hpp"

#include <algorithm>
#include <cctype>

#include <magic_enum.hpp>

#include "roq/logging.hpp"

#include "roq/bybit/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {

// === HELPERS ===

namespace {
auto create_api() {
  std::string value{flags::Flags::api()};
  std::transform(std::begin(value), std::end(value), std::begin(value), ::toupper);
  auto result = magic_enum::enum_cast<tools::API>(value);
  if (!result.has_value())
    log::fatal(R"(Unexpected: api="{}")"sv, value);
  return *result;
}

auto create_category(auto api) -> json::Category {
  switch (api) {
    using enum tools::API;
    case UNDEFINED:
      break;
    case SPOT:
      return json::Category::SPOT;
    case LINEAR:
      return json::Category::LINEAR;
    case INVERSE:
      return json::Category::INVERSE;
    case OPTION:
      return json::Category::OPTION;
  }
  log::fatal("Unexpected"sv);
}
}  // namespace

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher)
    : dispatcher{dispatcher}, rate_limiter{flags::Flags::request_limit(), flags::Flags::request_limit_interval()},
      api{create_api()}, category{create_category(api)}, symbols{flags::Flags::ws_max_subscriptions_per_stream()} {
}

}  // namespace bybit
}  // namespace roq
