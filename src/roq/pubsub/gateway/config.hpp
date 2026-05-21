/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/ranges.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/logging.hpp"
#include "roq/server.hpp"

#include "roq/server/config/dispatcher.hpp"
#include "roq/server/config/reader.hpp"

#include "roq/pubsub/gateway/settings.hpp"

namespace roq {
namespace pubsub {
namespace gateway {

struct ROQ_PUBLIC Config final : public server::config::Dispatcher, public server::config::Reader::Handler {
  explicit Config(Settings const &);

  Config(Config const &) = delete;

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

 private:
  std::string const exchange_;
  GatewaySettings const gateway_settings_;

 public:
  server::config::Users users;
  server::config::Symbols symbols;
  server::config::Accounts accounts;
  Account master_account_;
  server::config::RateLimits rate_limits;
};

}  // namespace gateway
}  // namespace pubsub
}  // namespace roq

template <>
struct fmt::formatter<roq::pubsub::gateway::Config> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::pubsub::gateway::Config const &value, format_context &context) const {
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
