/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <fmt/ranges.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/logging.hpp"
#include "roq/server.hpp"

#include "roq/server/config/dispatcher.hpp"
#include "roq/server/config/reader.hpp"

namespace roq {
namespace bybit {

struct Config final : public server::config::Dispatcher, public server::config::Reader::Handler {
  Config();

  Account const &get_master_account() const;

  std::string const &get_api_key(Account const &) const;
  std::string const &get_passphrase(Account const &) const;
  std::string const &get_secret(Account const &) const;

 protected:
  // server::config::Dispatcher

  void dispatch(server::config::Handler &) const override;

  // server::config::Reader::Handler

  void operator()(server::config::Symbols &&) override;
  void operator()(server::config::Account &&) override;
  void operator()(server::config::User &&) override;
  void operator()(server::config::RateLimit &&) override;
  void operator()(server::config::RequestTemplate, std::string_view const &label, toml::table &) override;
  void operator()(std::string_view const &key, toml::node &) override;

 public:
  server::config::Users users;
  server::config::Symbols symbols;
  server::config::Accounts accounts;
  Account master_account_;
  server::config::RateLimits rate_limits;
};

}  // namespace bybit
}  // namespace roq

template <>
struct fmt::formatter<roq::bybit::Config> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(roq::bybit::Config const &value, Context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(symbols={}, )"
        R"(accounts=[{}], )"
        R"(master_account="{}", )"
        R"(users=[{}], )"
        R"(rate_limits=[{}])"
        R"(}})"sv,
        value.symbols,
        fmt::join(value.accounts, ", "sv),
        value.master_account_,
        fmt::join(value.users, ", "sv),
        fmt::join(value.rate_limits, ", "sv));
  }
};
