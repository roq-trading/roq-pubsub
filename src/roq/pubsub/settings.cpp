/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/pubsub/settings.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

Settings::Settings(args::Parser const &args)
    : server::flags::Settings{args, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER}, flags::Flags{flags::Flags::create()} {
  log::debug("settings={}"sv, *this);
}

}  // namespace pubsub
}  // namespace roq
