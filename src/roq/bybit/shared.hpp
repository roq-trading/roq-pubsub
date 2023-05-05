/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/core/memory.hpp"
#include "roq/core/symbols.hpp"

#include "roq/core/limit/rate_limiter.hpp"

#include "roq/bybit/tools/api.hpp"

#include "roq/bybit/json/category.hpp"

namespace roq {
namespace bybit {

struct Shared final {
  explicit Shared(server::Dispatcher &);

  Shared(Shared &&) = default;
  Shared(Shared const &) = delete;

  auto discard_symbol(std::string_view const &name) const { return dispatcher.discard_symbol(name); }

  template <typename... Args>
  auto update_order(Args &&...args) {
    return dispatcher.update_order(std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto find_order(Args &&...args) {
    return dispatcher.find_order(std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto operator()(Args &&...args) {
    return dispatcher(std::forward<Args>(args)...);
  }

 public:
  std::vector<MBPUpdate> bids, asks;
  std::vector<Trade> trades;
  std::vector<Fill> fills;

 public:
  server::Dispatcher &dispatcher;

 public:
  core::limit::RateLimiter rate_limiter;

  tools::API const api;
  json::Category const category;

  core::Symbols symbols;
};

}  // namespace bybit
}  // namespace roq
