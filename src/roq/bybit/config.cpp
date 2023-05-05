/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/config.hpp"

#include <utility>

#include "roq/logging.hpp"

#include "roq/utils/safe_cast.hpp"

#include "roq/bybit/flags.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {

namespace {
auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::FUNDS,
};
auto const MBP_ALLOW_REMOVE_NON_EXISTING = true;
auto const OMS_REQUEST_ID_TYPE = RequestIdType::BASE64;
}  // namespace

// === IMPLEMENTATION ===

Config::Config() {
  server::config::Reader::parse_file(*this);
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
  handler(Flags::exchange());
  handler(symbols);
  for (auto &iter : accounts)
    handler(iter.second);
  for (auto &user : users)
    handler(user);
  auto gateway_settings = GatewaySettings{
      .supports = SUPPORTS,
      .mbp_max_depth = utils::safe_cast{flags::Flags::ws_mbp_depth()},
      .mbp_tick_size_multiplier = NaN,
      .mbp_min_trade_vol_multiplier = NaN,
      .mbp_allow_remove_non_existing = MBP_ALLOW_REMOVE_NON_EXISTING,
      .mbp_allow_price_inversion = Flags::mbp_allow_price_inversion(),
      .mbp_checksum = server::Flags::cache_mbp_checksum(),
      .oms_download_has_state = {},
      .oms_download_has_routing_id = {},
      .oms_request_id_type = OMS_REQUEST_ID_TYPE,
  };
  handler(gateway_settings);
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

}  // namespace bybit
}  // namespace roq
