/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/bybit/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace bybit {
namespace json {

std::string_view strip_symbol(std::string_view const &topic) {
  auto sep = topic.find_last_of('.');
  if (sep == topic.npos || sep == std::size(topic))
    return topic;
  return topic.substr(sep + 1);
}

namespace {
auto map_order_type(auto order_type, auto execution_instructions) -> json::OrderType {
  switch (order_type) {
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
}  // namespace

std::string_view place_order(
    std::string &buffer,
    roq::CreateOrder const &create_order,
    oms::Order const &order,
    std::string_view const &request_id,
    Category category) {
  buffer.clear();
  auto side = map(create_order.side);
  auto order_type = map_order_type(create_order.order_type, create_order.execution_instructions);
  auto time_in_force = map(create_order.time_in_force);
  auto reduce_only = create_order.execution_instructions.has(ExecutionInstruction::DO_NOT_INCREASE);
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}",)"
      R"("side":"{}",)"
      R"("orderType":"{}",)"
      R"("qty":"{}",)"
      R"("timeInForce":"{}",)"
      R"("reduceOnly":{})"sv,
      category.as_raw_text(),
      create_order.symbol,
      side.as_raw_text(),
      order_type.as_raw_text(),
      utils::Number{create_order.quantity, order.quantity_decimals},
      time_in_force.as_raw_text(),
      reduce_only);
  if (!std::isnan(create_order.price))
    fmt::format_to(
        std::back_inserter(buffer), R"(,"price":"{}")"sv, utils::Number{create_order.price, order.price_decimals});
  fmt::format_to(
      std::back_inserter(buffer),
      R"(,"orderLinkId":"{}")"
      R"(}})"sv,
      request_id);
  return buffer;
}

std::string_view amend_order(
    std::string &buffer,
    roq::ModifyOrder const &modify_order,
    oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    std::string_view const &previous_request_id,
    Category category) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"sv,
      category.as_raw_text(),
      order.symbol);
  if (!std::isnan(modify_order.price))
    fmt::format_to(
        std::back_inserter(buffer), R"(,"price":"{}")"sv, utils::Number{modify_order.price, order.price_decimals});
  if (!std::isnan(modify_order.quantity))
    fmt::format_to(
        std::back_inserter(buffer), R"(,"qty":"{}")"sv, utils::Number{modify_order.quantity, order.quantity_decimals});
  if (!std::empty(order.external_order_id)) {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderId":"{}")"
        R"(}})"sv,
        order.external_order_id);
  } else {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderLinkId":"{}")"
        R"(}})"sv,
        previous_request_id);  // XXX not correct -- we need original request_id
  }
  return buffer;
}

std::string_view cancel_order(
    std::string &buffer,
    roq::CancelOrder const &,
    oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    std::string_view const &previous_request_id,
    Category category) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"sv,
      category.as_raw_text(),
      order.symbol);
  if (!std::empty(order.external_order_id)) {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderId":"{}")"
        R"(}})"sv,
        order.external_order_id);
  } else {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderLinkId":"{}")"
        R"(}})"sv,
        previous_request_id);  // XXX not correct -- we need original request_id
  }
  return buffer;
}

std::string_view cancel_all_orders(
    std::string &buffer,
    roq::CancelAllOrders const &,
    [[maybe_unused]] std::string_view const &request_id,
    std::string_view const &symbol,
    Category category) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"
      R"(}})"sv,
      category.as_raw_text(),
      symbol);
  return buffer;
}

Error map_error(int32_t ret_code) {
  switch (ret_code) {
    case 0:
      return {};
    case 5019:  // User-service error
      break;
    case 5004:  // Upstream server timeout
      break;
    case 7001:  // Grpc request filed type error
      break;
    case 10003:  // Invalid api_key
      break;
    case 10006:  // Too many visits
      break;
    case 10001:  // Params error
      return Error::INVALID_REQUEST_ARGS;
    case 10002:  // Invalid request, please check your timestamp and recv_window param
      break;
    case 10004:  // Error sign
      break;
    case 10005:  // Permission denied
      break;
    case 33004:  // api_key expire
      break;
    case 12940:  // Connection reset
      break;
    case 12999:  // Server Error
      break;
    case 12800:  // Invalid request
      return Error::INVALID_REQUEST_ARGS;
    case 12801:  // Invalid JSON
      return Error::INVALID_REQUEST_ARGS;
    case 12802:  // Invalid event
      break;
    case 12803:  // Event required
      break;
    case 12804:  // Invalid topic
      break;
    case 12805:  // Topic required
      break;
    case 12806:  // Params required
      return Error::INVALID_REQUEST_ARGS;
    case 12807:  // Period required
      break;
    case 12808:  // Invalid period
      break;
    case 12809:  // Invalid Symbols
      return Error::INVALID_SYMBOL;
    case 12810:  // Not supported symbols
      return Error::INVALID_SYMBOL;
    case 12811:  // There is no such broker.
      break;
    case 12812:  // DumpScale required
      break;
    case 12813:  // Invalid exchangeId
      return Error::INVALID_EXCHANGE;
    case 12814:  // Param %s should be %s.
      return Error::INVALID_REQUEST_ARGS;
    case 12815:  // Symbol required
      return Error::INVALID_SYMBOL;
    case 12816:  // Timeout. Retry later.
      return Error::TIMEOUT;
    case 12817:  // No business with any exchange.
      break;
    case 12818:  // Client disconnected.
      break;
    case 12819:  // OrgId required.
      return Error::INVALID_REQUEST_ID;
    case 12820:  // OrgId must be a number.
      return Error::INVALID_REQUEST_ID;
    case 12821:  // DumpScale error.
      break;
    case 12822:  // Index name error.
      break;
    case 12823:  // Parameter error
      return Error::INVALID_REQUEST_ARGS;
    case 12824:  // Parameter %s [%s]
      return Error::INVALID_REQUEST_ARGS;
    case 12825:  // The number of each symbol request cannot exceed 300
      break;
    case 12001:  // Request failed to be processed. Please try again.
      break;
    case 12007:  // Response timeout from backend server. Delivery and request status unknown.
      break;
    case 12014:  // Trading pairs not supported.
      return Error::INVALID_SYMBOL;
    case 12005:  // Too many new orders. Please lower request frequency.
      break;
    case 12016:  // Service not available.
      break;
    case 12032:  // Fusing error
      break;
    case 12114:  // NETWORK_BUSY
      break;
    case 12015:  // UID_NOT_EXIST
      break;
    case 12011:  // INNOVATION_BUY_FAILED_BY_QUOTA
      break;
    case 12010:  // ETP_BUY_FAILED_BY_QUOTA
      break;
    case 12012:  // ETP_NAV_IS_ABNORMAL
      break;
    case 12017:  // CROSS_MARGIN_PRE_BUY_QUOTA_FAILED
      break;
    case 10020:  // CROSS_MARGIN_PRE_SELL_QUOTA_FAILED
      break;
    case 12019:  // Your account has been restricted for trades. If you have any questions, please email us at
                 // support@bybit.com
      break;
    case 12124:  // Order amount is too big
      return Error::INSUFFICIENT_FUNDS;
    case 12031:  // The feature has been suspended
      break;
    case 12105:  // Empty parameter.
      break;
    case 12115:  // Invalid timeInForce.
      return Error::INVALID_TIME_IN_FORCE;
    case 12116:  // Invalid orderType.
      return Error::INVALID_ORDER_TYPE;
    case 12117:  // Invalid direction.
      return Error::INVALID_SIDE;
    case 12121:  // Invalid symbol.
      return Error::INVALID_SYMBOL;
    case 12130:  // Invalid parameter sent.
      return Error::INVALID_REQUEST_ARGS;
    case 12131:  // Insufficient balance
      return Error::INSUFFICIENT_FUNDS;
    case 12132:  // Order price exceeded upper limit.
      return Error::INVALID_PRICE;
    case 12133:  // Order price exceeded lower limit.
      return Error::INVALID_PRICE;
    case 12134:  // Order price has too many decimals.
      return Error::INVALID_PRICE;
    case 12135:  // Order quantity exceeded upper limit.
      return Error::INVALID_QUANTITY;
    case 12136:  // Order quantity exceeded lower limit.
      return Error::INVALID_QUANTITY;
    case 12137:  // Order quantity has too many decimals.
      return Error::INVALID_QUANTITY;
    case 12138:  // Order price exceeded limits.
      return Error::INVALID_PRICE;
    case 12139:  // Order has been filled.
      return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
    case 12140:  // Order value exceeded lower limit.
      break;
    case 12141:  // Duplicate clientOrderId.
      return Error::INVALID_REQUEST_ID;
    case 12142:  // Order has been canceled.
      return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
    case 12143:  // Order not found.
      return Error::UNKNOWN_ORDER_ID;
    case 12144:  // Order being cancelled. Operation not supported.
      return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
    case 12145:  // Order cannot be canceled.
      return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
    case 12146:  // Order creation timeout.
      break;
    case 12147:  // Order cancellation timeout.
      break;
    case 12148:  // Market order amount decimal too long
      return Error::INVALID_QUANTITY;
    case 12149:  // Create order failed
      break;
    case 12150:  // Cancel order failed
      break;
    case 12151:  // The trading pair is not open yet
      break;
    case 12156:  // Order quantity invalid
      return Error::INVALID_QUANTITY;
    case 12157:  // The trading pair is not available for api trading
      break;
    case 12158:  // create limit maker order failed
      break;
    case 12159:  // Market Order is not supported within the first %s minutes of newly launched pairs due to risk
                 // control.
      break;
    case 12190:  // Cancel order has been finished.
      break;
    case 12191:  // Can not cancel order, please try again later.
      break;
    case 12192:  // Order price cannot be higher than %s .
      return Error::INVALID_PRICE;
    case 12193:  // Buy order price cannot be higher than %s.
      return Error::INVALID_PRICE;
    case 12194:  // Sell order price cannot be lower than %s.
      return Error::INVALID_PRICE;
    case 12195:  // Please note that your order may not be filled
      break;
    case 12196:  // Please note that your order may not be filled
      break;
    case 12197:  // Your order quantity to buy is too large. The filled price may deviate significantly from the market
                 // price. Please try again
      return Error::INVALID_QUANTITY;
      break;
    case 12198:  // Your order quantity to sell is too large. The filled price may deviate significantly from the market
                 // price. Please try again
      return Error::INVALID_QUANTITY;
    case 12199:  // Your order quantity to buy is too large. The filled price may deviate significantly from the nav.
                 // Please try again.
      return Error::INVALID_QUANTITY;
    case 12200:  // Your order quantity to sell is too large. The filled price may deviate significantly from the nav.
                 // Please try again.
      return Error::INVALID_QUANTITY;
    case 12201:  // Invalid orderCategory parameter
      break;
    case 12202:  // Please enter the TP/SL price
      break;
    case 12203:  // trigger price cannot be higher than 110% price
      return Error::INVALID_PRICE;
    case 12204:  // trigger price cannot be lower than 90% of qty
      return Error::INVALID_PRICE;
    case 12205:  // CROSS_MARGIN_USER_NOT_ALLOW
      break;
    case 12206:  // Stop_limit Order is not supported within the first 5 minutes of newly launched pairs
      break;
    case 12210:  // New order rejected.
      break;
    case 12211:  // Cancelation rejected.
      break;
    case 12213:  // Order does not exist.
      break;
    case 12217:  // Only LIMIT-MAKER order is supported for the current pair.
      break;
    case 12218:  // The LIMIT-MAKER order is rejected due to invalid price.
      break;
    case 12221:  // This coin does not exist
      return Error::INVALID_SYMBOL;
    case 12222:  // Too many requests in this time frame
      break;
    case 12223:  // Your Spot Account with Institutional Lending triggers an alert or liquidation
      break;
    case 12224:  // You're not a user of the Innovation Zone
      break;
    case 12225:  // You've failed to check the Risk Alert for Leveraged Tokens
      break;
    case 12226:  // Your Spot Account for Margin Trading is being liquidated
      break;
    case 12227:  // This feature is not supported
      break;
    case 12228:  // The purchase amount of each order exceeds the estimated maximum purchase amount
      return Error::INVALID_QUANTITY;
    case 12229:  // The sell quantity per order exceeds the estimated maximum sell quantity
      return Error::INVALID_QUANTITY;
    case 12230:  // The reserved quota for Spot Margin Trading does not meet the LTV ratio requirement
      break;
    case 12231:  // Users with Spot Margin Trading are not allowed to perform Block Trades
      break;
    case 12232:  // Users with Institutional Lending are not allowed to perform Block Trades
      break;
    case 12233:  // Users with Leverage Tokens are not allowed to perform Block Trades
      break;
    case 12234:  // System Error
      break;
    case 12400:  // The serialNum is already in use.
      break;
    case 12401:  // Daily purchase limit has been exceeded. Please try again later.
      break;
    case 12402:  // There's a large number of purchase orders. Please try again later.
      break;
    case 12403:  // Insufficient available balance. Please make a deposit and try again.
      return Error::INSUFFICIENT_FUNDS;
    case 12404:  // Daily redemption limit has been exceeded. Please try again later.
      break;
    case 12405:  // There's a large number of redemption orders. Please try again later.
      break;
    case 12406:  // Insufficient available balance. Please make a deposit and try again.
      return Error::INSUFFICIENT_FUNDS;
    case 12407:  // Order not found.
      return Error::UNKNOWN_ORDER_ID;
    case 12408:  // Purchase period hasn't started yet.
      break;
    case 12409:  // Purchase amount has exceeded the upper limit.
      break;
    case 12410:  // You haven't passed the quiz yet! To purchase and/or redeem an LT, please complete the quiz first.
      break;
    case 12412:  // Redemption period hasn't started yet.
      break;
    case 12413:  // Redemption amount has exceeded the upper limit.
      break;
    case 12414:  // Purchase of the LT has been temporarily suspended.
      break;
    case 12415:  // Redemption of the LT has been temporarily suspended.
      break;
    case 12416:  // Invalid format. Please check the length and numeric precision.
      return Error::INVALID_REQUEST_ARGS;
    case 12417:  // Failed to place order：Exceed the maximum position limit of leveraged tokens
      break;
    case 12601:  // Query user repay history error
      break;
    case 12602:  // Query user account info error
      break;
    case 12603:  // Query user loan history error
      break;
    case 12604:  // Query order history start time exceeds end time
      break;
    case 12605:  // Failed to borrow
      break;
    case 12606:  // Repayment Failed
      break;
    case 12607:  // User not found
      break;
    case 12608:  // You haven't enabled Cross Margin Trading yet. To do so
      break;
    case 12609:  // You haven't enabled Cross Margin Trading yet. To do so
      break;
    case 12610:  // Failed to locate the coins to borrow
      break;
    case 12611:  // Cross Margin Trading not yet supported by the selected coin
      break;
    case 12612:  // Pair not available
      break;
    case 12613:  // Cross Margin Trading not yet supported by the selected pair
      break;
    case 12614:  // Repeated repayment requests
      break;
    case 12615:  // Insufficient available balance
      break;
    case 12616:  // No repayment required
      break;
    case 12617:  // Repayment amount has exceeded the total liability
      break;
    case 12618:  // Settlement in progress
      break;
    case 12619:  // Liquidation in progress
      break;
    case 12620:  // Failed to locate repayment history
      break;
    case 12621:  // Repeated borrowing requests
      break;
    case 12622:  // Coins to borrow not generally available yet
      break;
    case 12623:  // Pair to borrow not generally available yet
      break;
    case 12624:  // Invalid user status
      break;
    case 12625:  // Amount to borrow cannot be lower than the min. amount to borrow (per transaction)
      break;
    case 12626:  // Amount to borrow cannot be larger than the max. amount to borrow (per transaction)
      break;
    case 12627:  // Amount to borrow cannot be higher than the max. amount to borrow per user
      break;
    case 12628:  // Amount to borrow has exceeded Bybit's max. amount to borrow
      break;
    case 12629:  // Amount to borrow has exceeded the user's estimated max. amount to borrow
      break;
    case 12630:  // Query user loan info error
      break;
    case 12631:  // Number of decimals has exceeded the maximum precision
      break;
    case 12632:  // Number of decimals has exceeded the maximum precision
      break;
  }
  return Error::UNKNOWN;
}

}  // namespace json
}  // namespace bybit
}  // namespace roq
