/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include "roq/service.hpp"

namespace roq {
namespace pubsub {

struct Application final : public roq::Service {
  using roq::Service::Service;

 protected:
  int main(int, char **) override;
};

}  // namespace pubsub
}  // namespace roq
