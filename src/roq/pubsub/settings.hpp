/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <fmt/compile.h>
#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/pubsub/flags/flags.hpp"

namespace roq {
namespace pubsub {

struct Settings final : public server::flags::Settings, public flags::Flags {
  explicit Settings(args::Parser const &);
};

}  // namespace pubsub
}  // namespace roq

template <>
struct fmt::formatter<roq::pubsub::Settings> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(roq::pubsub::Settings const &value, Context &context) const {
    using namespace fmt::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(server={})"
        R"(}})"_cf,
        value.exchange,
        static_cast<roq::server::Settings const &>(value));
  }
};
