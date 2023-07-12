/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/pubsub/settings.hpp"

#include "roq/logging.hpp"

#include "roq/pubsub/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

Settings::Settings(args::Parser const &args)
    : server::flags::Settings{args, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER}, exchange{flags::Flags::exchange()} {
  log::debug("settings={}"sv, *this);
}

}  // namespace pubsub
}  // namespace roq
