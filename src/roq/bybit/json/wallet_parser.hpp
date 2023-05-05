/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/buffer.hpp"

#include "roq/core/json/buffer.hpp"

#include "roq/bybit/json/wallet.hpp"

namespace roq {
namespace bybit {
namespace json {

struct WalletParser final {
  struct Handler {
    virtual void operator()(Trace<json::Wallet> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::Buffer &, TraceInfo const &);
};

}  // namespace json
}  // namespace bybit
}  // namespace roq
