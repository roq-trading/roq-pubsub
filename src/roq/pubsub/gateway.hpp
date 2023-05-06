/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include "roq/server.hpp"

#include "roq/pubsub/config.hpp"

namespace roq {
namespace pubsub {

struct Gateway final : public server::Handler {
  Gateway(server::Dispatcher &, Config const &, io::Context &);

 protected:
  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  uint16_t operator()(Event<CreateOrder> const &, oms::Order const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

  void operator()(metrics::Writer &) override;

 private:
  server::Dispatcher &dispatcher_;
};

}  // namespace pubsub
}  // namespace roq
