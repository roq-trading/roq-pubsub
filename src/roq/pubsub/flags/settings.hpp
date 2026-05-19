/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/pubsub/flags/flags.hpp"

namespace roq {
namespace pubsub {
namespace flags {

struct ROQ_PUBLIC Settings final : public server::flags::Settings, public flags::Flags {
  explicit Settings(args::Parser const &);
};

}  // namespace flags
}  // namespace pubsub
}  // namespace roq

template <>
struct fmt::formatter<roq::pubsub::flags::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::pubsub::flags::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(server={})"
        R"(}})"sv,
        value.exchange,
        static_cast<roq::server::Settings const &>(value));
  }
};
