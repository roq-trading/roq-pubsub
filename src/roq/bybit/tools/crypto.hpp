/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

#include <roq/core/hash/sha256.hpp>

#include "roq/core/mac/hmac.hpp"

namespace roq {
namespace bybit {
namespace tools {

struct Crypto final {
  Crypto(std::string_view const &key, std::string_view const &secret, std::chrono::milliseconds recv_window);

  Crypto(Crypto &&) = delete;
  Crypto(Crypto const &) = delete;

  auto const &get_key() const { return key_; }

  std::string create_signature_v2(std::chrono::milliseconds expires);

  std::string create_headers_v2(
      std::string_view const &path,
      std::string_view const &query,
      std::string_view const &body,
      std::chrono::milliseconds now);

 private:
  using MAC = core::mac::HMAC<core::hash::SHA256>;
  using Digest = std::array<std::byte, MAC::DIGEST_LENGTH>;

  std::string const key_;
  MAC mac_;
  Digest digest_;
  std::string const passphrase_;
  std::string const signed_passphrase_;
  std::chrono::milliseconds const recv_window_;
};

}  // namespace tools
}  // namespace bybit
}  // namespace roq
