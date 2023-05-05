/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/json/wallet_parser.hpp"

#include "roq/core/json/array_parser.hpp"
#include "roq/core/json/parser.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {
namespace json {

// === IMPLEMENTATION ===

bool WalletParser::dispatch(
    Handler &handler, std::string_view const &message, core::Buffer &buffer, TraceInfo const &trace_info) {
  core::json::Parser parser{message};
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::Object>(root)) {
    if (key.compare("result"sv) == 0) {
      for (auto [key_2, value_2] : std::get<core::json::Object>(value)) {
        if (key_2.compare("list"sv) == 0) {
          for (auto item : std::get<core::json::Array>(value_2)) {
            Wallet wallet{item, buffer};
            create_trace_and_dispatch(handler, trace_info, wallet);
          }
          return true;
        }
      }
    }
  }
  return false;
}

}  // namespace json
}  // namespace bybit
}  // namespace roq
