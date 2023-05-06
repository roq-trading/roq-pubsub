/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/pubsub/settings.hpp"

#include "roq/pubsub/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

Settings Settings::create(server::Type type) {
  auto settings = server::create_settings(type, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER);
  return {settings};
}

}  // namespace pubsub
}  // namespace roq
