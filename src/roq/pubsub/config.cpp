/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/pubsub/config.hpp"

#include <utility>

#include "roq/logging.hpp"

#include "roq/pubsub/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

// === CONSTANTS ===

namespace {
auto const OMS_REQUEST_ID_TYPE = RequestIdType::BASE64;
}

// === HELPERS ===

namespace {
auto create_gateway_settings([[maybe_unused]] auto &settings) -> GatewaySettings {
  return {
      .supports = {},
      .mbp_max_depth = {},
      .mbp_tick_size_multiplier = NaN,
      .mbp_min_trade_vol_multiplier = NaN,
      .mbp_allow_remove_non_existing = {},
      .mbp_allow_price_inversion = {},
      .mbp_checksum = {},
      .oms_download_has_state = {},
      .oms_download_has_routing_id = {},
      .oms_request_id_type = OMS_REQUEST_ID_TYPE,
      .oms_cancel_all_orders = {},
  };
}
}  // namespace

// === IMPLEMENTATION ===

Config::Config(Settings const &settings)
    : exchange_{settings.exchange}, gateway_settings_{create_gateway_settings(settings)} {
  server::config::Reader::parse_file(*this, settings);
  log::info<1>("config={}"sv, *this);
}

Account const &Config::get_master_account() const {
  return master_account_;
}

std::string const &Config::get_api_key(Account const &account) const {
  auto iter = accounts.find(account);
  if (iter == std::end(accounts))
    log::fatal(R"(Unknown account="{}")"sv, account);
  return (*iter).second.login;
}

std::string const &Config::get_passphrase(Account const &account) const {
  auto iter = accounts.find(account);
  if (iter == std::end(accounts))
    log::fatal(R"(Unknown account="{}")"sv, account);
  return (*iter).second.password;
}

std::string const &Config::get_secret(Account const &account) const {
  auto iter = accounts.find(account);
  if (iter == std::end(accounts))
    log::fatal(R"(Unknown account="{}")"sv, account);
  return (*iter).second.secret;
}

void Config::dispatch(server::config::Handler &handler) const {
  handler(exchange_);
  handler(symbols);
  for (auto &iter : accounts)
    handler(iter.second);
  for (auto &user : users)
    handler(user);
  handler(gateway_settings_);
  for (auto &iter : rate_limits)
    handler(iter.second);
}

void Config::operator()(server::config::Symbols &&symbols) {
  (*this).symbols = std::move(symbols);
}

void Config::operator()(server::config::Account &&account) {
  if (account.master)
    master_account_ = account.name;
  accounts.emplace(account.name, std::move(account));
}

void Config::operator()(server::config::User &&user) {
  users.emplace_back(std::move(user));
}

void Config::operator()(server::config::RateLimit &&rate_limit) {
  rate_limits.emplace(rate_limit.name, std::move(rate_limit));
}

void Config::operator()(
    server::config::RequestTemplate, [[maybe_unused]] std::string_view const &label, toml::table &) {
  log::fatal("Unexpected: request templates not supported"sv);
}

void Config::operator()(std::string_view const &key, toml::node &) {
  log::warn(R"(Unexpected: key="{}")"sv, key);
}

}  // namespace pubsub
}  // namespace roq
