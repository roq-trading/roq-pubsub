/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/pubsub/application.hpp"

#include "roq/pubsub/config.hpp"
#include "roq/pubsub/gateway.hpp"
#include "roq/pubsub/settings.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  Config config{settings};
  auto context = server::create_io_context(settings);
  server::PubSub<Gateway>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace pubsub
}  // namespace roq
