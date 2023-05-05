/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include "roq/oms/order.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/charconv.hpp"
#include "roq/core/charconv/datetime.hpp"

#include "roq/bybit/json/event_type.hpp"

#include "roq/bybit/json/category.hpp"

#include "roq/bybit/json/contract_type.hpp"
#include "roq/bybit/json/options_type.hpp"
#include "roq/bybit/json/status.hpp"

#include "roq/bybit/json/order_status.hpp"
#include "roq/bybit/json/order_type.hpp"
#include "roq/bybit/json/side.hpp"
#include "roq/bybit/json/time_in_force.hpp"

namespace roq {
namespace bybit {
namespace json {

template <typename T>
inline void update(T &result, core::json::Value const &value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(std::chrono::milliseconds &result, core::json::Value const &value) {
  return std::visit(
      overloaded{
          [&](core::json::Null const &) { result = std::chrono::milliseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = std::chrono::milliseconds{value}; },
          [&](double value) { result = std::chrono::milliseconds{static_cast<int64_t>(value * 1e3)}; },
          [&](std::string_view const &value) {
            auto tmp = core::from_chars<int64_t>(value);
            // note! have seen 1000
            result = std::chrono::milliseconds{tmp > 1000 ? tmp : 0};
          },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

template <>
inline void update(std::chrono::microseconds &result, core::json::Value const &value) {
  return std::visit(
      overloaded{
          [&](core::json::Null const &) { result = std::chrono::microseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = std::chrono::microseconds{value}; },
          [&](double value) { result = std::chrono::microseconds{static_cast<int64_t>(value * 1e6)}; },
          [&](std::string_view const &value) {
            auto tmp = core::from_chars<double>(value);
            result = std::chrono::microseconds{static_cast<int64_t>(tmp * 1e6)};
          },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

template <>
inline void update(std::chrono::nanoseconds &result, core::json::Value const &value) {
  return std::visit(
      overloaded{
          [&](core::json::Null const &) { result = std::chrono::nanoseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = std::chrono::nanoseconds{value}; },
          [&](double value) { result = std::chrono::nanoseconds{static_cast<int64_t>(value * 1e9)}; },
          [&](std::string_view const &value) {
            auto tmp = core::from_chars<double>(value);
            result = std::chrono::nanoseconds{static_cast<int64_t>(tmp * 1e9)};
          },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

// update type

inline UpdateType map(json::EventType event_type) {
  switch (event_type) {
    using enum json::EventType::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case ERROR:
      break;
    case SNAPSHOT:
      return UpdateType::SNAPSHOT;
    case DELTA:
      return UpdateType::INCREMENTAL;
    case COMMAND_RESP:
      break;
  }
  return {};
}

// security type

inline SecurityType map(json::ContractType contract_type, json::OptionsType options_type) {
  switch (options_type) {
    using enum json::OptionsType::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case CALL:
    case PUT:
      return SecurityType::OPTION;
  }
  switch (contract_type) {
    using enum json::ContractType::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case INVERSE_PERPETUAL:
    case LINEAR_PERPETUAL:
      return SecurityType::SWAP;
    case LINEAR_FUTURES:
    case INVERSE_FUTURES:
      return SecurityType::FUTURES;
  }
  return SecurityType::SPOT;
}

// options type

inline roq::OptionType map(json::OptionsType value) {
  switch (value) {
    using enum json::OptionsType::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case CALL:
      return roq::OptionType::CALL;
    case PUT:
      return roq::OptionType::PUT;
  }
  return {};
}

// (trading) status

inline roq::TradingStatus map(json::Status value) {
  switch (value) {
    using enum json::Status::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case PRE_LAUNCH:
      break;
    case TRADING:
      return roq::TradingStatus::OPEN;
    case SETTLING:
      break;
    case DELIVERING:
      break;
    case CLOSED:
      return roq::TradingStatus::CLOSE;
  }
  return {};
}

// side

inline roq::Side map(json::Side value) {
  switch (value) {
    using enum json::Side::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case NONE:
      break;
    case BUY:
      return roq::Side::BUY;
    case SELL:
      return roq::Side::SELL;
  }
  return {};
}

inline json::Side map(roq::Side value) {
  switch (value) {
    using enum roq::Side;
    case UNDEFINED:
      break;
    case BUY:
      return json::Side::BUY;
    case SELL:
      return json::Side::SELL;
  }
  return {};
}

// order type
// XXX need maker

inline roq::OrderType map(json::OrderType value) {
  switch (value) {
    using enum json::OrderType::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case UNKNOWN:
      break;
    case MARKET:
      return roq::OrderType::MARKET;
    case LIMIT:
      return roq::OrderType::LIMIT;
  }
  return {};
}

inline json::OrderType map(roq::OrderType value) {
  switch (value) {
    using enum roq::OrderType;
    case UNDEFINED:
      break;
    case MARKET:
      return json::OrderType::MARKET;
    case LIMIT:
      return json::OrderType::LIMIT;
  }
  return {};
}

// time in force

inline roq::TimeInForce map(json::TimeInForce value) {
  switch (value) {
    using enum json::TimeInForce::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case GTC:
      return roq::TimeInForce::GTC;
    case FOK:
      return roq::TimeInForce::FOK;
    case IOC:
      return roq::TimeInForce::IOC;
  }
  return {};
}

inline json::TimeInForce map(roq::TimeInForce value) {
  switch (value) {
    using enum roq::TimeInForce;
    case UNDEFINED:
      break;
    case GFD:
      break;
    case GTC:
      return json::TimeInForce::GTC;
    case OPG:
      break;
    case IOC:
      return json::TimeInForce::IOC;
    case FOK:
      return json::TimeInForce::FOK;
    case GTX:
      break;
    case GTD:
      break;
    case AT_THE_CLOSE:
      break;
    case GOOD_THROUGH_CROSSING:
      break;
    case AT_CROSSING:
      break;
    case GOOD_FOR_TIME:
      break;
    case GFA:
      break;
    case GFM:
      break;
  }
  return {};
}

// order status

inline roq::OrderStatus map(json::OrderStatus value) {
  switch (value) {
    using enum json::OrderStatus::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case CREATED:
      return roq::OrderStatus::WORKING;
    case NEW:
      return roq::OrderStatus::WORKING;
    case REJECTED:
      return roq::OrderStatus::REJECTED;
    case PARTIALLY_FILLED:
      return roq::OrderStatus::WORKING;
    case PARTIALLY_FILLED_CANCELED:
      return roq::OrderStatus::CANCELED;
    case FILLED:
      return roq::OrderStatus::COMPLETED;
    case CANCELLED:
      return roq::OrderStatus::CANCELED;
    case UNTRIGGERED:
      break;
    case TRIGGERED:
      break;
    case DEACTIVATED:
      break;
    case ACTIVE:
      break;
  }
  return {};
}

extern std::string_view strip_symbol(std::string_view const &topic);

extern std::string_view place_order(
    std::string &buffer, roq::CreateOrder const &, oms::Order const &, std::string_view const &request_id, Category);

extern std::string_view amend_order(
    std::string &buffer,
    roq::ModifyOrder const &,
    oms::Order const &,
    std::string_view const &request_id,
    std::string_view const &previous_request_id,
    Category);

extern std::string_view cancel_order(
    std::string &buffer,
    roq::CancelOrder const &,
    oms::Order const &,
    std::string_view const &request_id,
    std::string_view const &previous_request_id,
    Category);

extern std::string_view cancel_all_orders(
    std::string &buffer,
    roq::CancelAllOrders const &,
    std::string_view const &request_id,
    std::string_view const &symbol,
    Category);

extern Error map_error(int32_t ret_code);

}  // namespace json
}  // namespace bybit
}  // namespace roq
