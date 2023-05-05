/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/core/timer_queue.hpp"

#include "roq/core/limit/queue.hpp"
#include "roq/core/limit/rate_limiter.hpp"

#include "roq/bybit/config.hpp"

#include "roq/bybit/tools/crypto.hpp"

namespace roq {
namespace bybit {

struct Account final {
  Account(Config const &, std::string_view const &name);

  Account(Account &&) = delete;
  Account(Account const &) = delete;

  std::string_view get_name() const { return name_; }
  std::string_view get_key() const { return crypto_.get_key(); }

  std::string create_signature(std::chrono::milliseconds expires);
  std::string create_headers(std::string_view const &path, std::string_view const &query, std::string_view const &body);

 private:
  std::string const name_;
  tools::Crypto crypto_;

 public:
  core::limit::RateLimiter rate_limiter;
  core::limit::Queue<std::pair<std::string, std::string>> request_queue;
};

}  // namespace bybit
}  // namespace roq
