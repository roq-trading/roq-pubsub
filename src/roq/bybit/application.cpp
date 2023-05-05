/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/application.hpp"

#include "roq/bybit/config.hpp"
#include "roq/bybit/gateway.hpp"

#include "roq/bybit/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {

// === HELPERS ===

namespace {
auto create_settings = []() {
  return server::Settings{
      .package_name = ROQ_PACKAGE_NAME,
      .build_number = ROQ_BUILD_NUMBER,
      .api = flags::Flags::api(),
      .type = server::Type::ORDER_MANAGEMENT,
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

}  // namespace bybit
}  // namespace roq
