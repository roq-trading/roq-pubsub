/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/pubsub/gateway/controller.hpp"

#include "roq/logging.hpp"

#include "roq/server/oms/exceptions.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {
namespace gateway {

// === IMPLEMENTATION ===

std::unique_ptr<server::Handler> Controller::create(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context) {
  return std::make_unique<Controller>(dispatcher, settings, config, context);
}

Controller::Controller(server::Dispatcher &dispatcher, Settings const &, Config const &, io::Context &) : dispatcher_{dispatcher} {
}

void Controller::operator()(Event<Start> const &) {
  log::info("Starting..."sv);
}

void Controller::operator()(Event<Stop> const &) {
  log::info("Stopping..."sv);
}

void Controller::operator()(Event<Timer> const &) {
}

void Controller::operator()(Event<Control> const &event) {
  auto &[message_info, control] = event;
  switch (control.action) {
    using enum Action;
    case UNDEFINED:
      assert(false);
      break;
    case ENABLE:
      dispatcher_(State::ENABLED);
      break;
    case DISABLE:
      dispatcher_(State::DISABLED);
      break;
  }
}

void Controller::operator()(Event<Connected> const &) {
}

void Controller::operator()(Event<Disconnected> const &) {
}

void Controller::operator()(Event<Subscribe> const &) {
}

uint16_t Controller::operator()(
    Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, [[maybe_unused]] std::string_view const &request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(
    Event<ModifyOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(
    Event<CancelOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Event<CancelAllOrders> const &, [[maybe_unused]] std::string_view const &request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Event<MassQuote> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Event<CancelQuotes> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Controller::operator()(metrics::Writer &) const {
}

}  // namespace gateway
}  // namespace pubsub
}  // namespace roq
