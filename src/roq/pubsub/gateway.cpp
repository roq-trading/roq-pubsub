/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/pubsub/gateway.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace pubsub {

// === IMPLEMENTATION ===

Gateway::Gateway(server::Dispatcher &, Settings const &, Config const &, io::Context &) {
}

void Gateway::operator()(Event<Start> const &) {
  log::info("Starting..."sv);
}

void Gateway::operator()(Event<Stop> const &) {
  log::info("Stopping..."sv);
}

void Gateway::operator()(Event<Timer> const &) {
}

void Gateway::operator()(Event<Connected> const &) {
}

void Gateway::operator()(Event<Disconnected> const &) {
}

uint16_t Gateway::operator()(
    Event<CreateOrder> const &, oms::Order const &, [[maybe_unused]] std::string_view const &request_id) {
  throw oms::NotSupported{"not supported"sv};
}

uint16_t Gateway::operator()(
    Event<ModifyOrder> const &,
    oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw oms::NotSupported{"not supported"sv};
}

uint16_t Gateway::operator()(
    Event<CancelOrder> const &,
    oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw oms::NotSupported{"not supported"sv};
}

uint16_t Gateway::operator()(Event<CancelAllOrders> const &, [[maybe_unused]] std::string_view const &request_id) {
  throw oms::NotSupported{"not supported"sv};
}

void Gateway::operator()(metrics::Writer &) {
}

}  // namespace pubsub
}  // namespace roq
