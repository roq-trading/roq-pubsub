/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/pubsub/application.hpp"

#include "roq/pubsub/config.hpp"
#include "roq/pubsub/gateway.hpp"

#include "roq/pubsub/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

// === HELPERS ===

namespace {
auto create_settings = []() {
  return server::Settings{
      .package_name = ROQ_PACKAGE_NAME,
      .build_number = ROQ_BUILD_NUMBER,
      .api = {},
      .type = server::Type::MARKET_DATA,  // XXX ???
  };
};
}  // namespace

// === IMPLEMENTATION ===

int Application::main(int, char **) {
  auto settings = create_settings();
  Config config;
  auto context = server::create_io_context();
  server::Trading<Gateway>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace pubsub
}  // namespace roq
