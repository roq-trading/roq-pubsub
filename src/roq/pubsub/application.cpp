/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/pubsub/application.hpp"

#include "roq/pubsub/flags/settings.hpp"

#include "roq/pubsub/gateway/config.hpp"
#include "roq/pubsub/gateway/controller.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  flags::Settings settings{args};
  gateway::Config config{settings};
  auto context = server::create_io_context(settings);
  server::PubSub2<gateway::Controller>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace pubsub
}  // namespace roq
