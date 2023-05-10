/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/pubsub/application.hpp"

#include "roq/pubsub/config.hpp"
#include "roq/pubsub/gateway.hpp"
#include "roq/pubsub/settings.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

// === CONSTANTS ===

namespace {
auto const TYPE = server::Type::PUBLISH_SUBSCRIBE;
}  // namespace

// === IMPLEMENTATION ===

int Application::main(int, char **) {
  Settings settings{TYPE};
  Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<Gateway>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace pubsub
}  // namespace roq
