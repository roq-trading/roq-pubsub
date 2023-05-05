set(TARGET_NAME "${PROJECT_NAME}-json")

set(SOURCES parser.cpp utils.cpp wallet_parser.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    account_info.json
    account_info_result.json
    account_type.json
    amend_order.json
    amend_order_result.json
    auth.json
    cancel_all_orders.json
    cancel_all_orders_result_item.json
    cancel_all_orders_result.json
    cancel_order.json
    cancel_order_result.json
    cancel_type.json
    category.json
    contract_type.json
    error.json
    event_type.json
    exec_type.json
    execution_2.json
    execution_data_item.json
    execution.json
    execution_result_item.json
    execution_result.json
    instrument_info.json
    instrument_info_result_item.json
    instrument_info_result.json
    leverage_filter.json
    lot_size_filter.json
    message.json
    open_orders.json
    open_orders_result_item.json
    open_orders_result.json
    operation.json
    options_type.json
    order_book_data.json
    order_book.json
    order_book_level.json
    order_data_item.json
    order.json
    order_status.json
    order_type.json
    ping.json
    place_order.json
    place_order_result.json
    position_data_item.json
    position_info.json
    position_info_result_item.json
    position_info_result.json
    position.json
    position_status.json
    price_filter.json
    public_trade_data.json
    public_trade.json
    reject_reason.json
    side.json
    status.json
    stop_order_type.json
    subscribe_data.json
    subscribe.json
    tick_direction.json
    tickers_data.json
    tickers.json
    time_in_force.json
    topic.json
    tp_sl_mode.json
    trigger_by.json
    wallet_balance_item.json
    wallet_balance.json
    wallet_balance_result_item.json
    wallet_balance_result.json
    wallet.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/bybit/json"
  OUTPUT_TYPE
  "json"
  FILE_TYPE
  "hpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target(autogen-json-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT
  AUTOGEN_SOURCES
  NAMESPACE
  "roq/bybit/json"
  OUTPUT_TYPE
  "json"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

roq_gitignore(OUTPUT ".gitignore" SOURCES "${TARGET_NAME}" ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library("${TARGET_NAME}" OBJECT ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-json-headers)

target_link_libraries("${TARGET_NAME}" roq-core::roq-core roq-logging::roq-logging fmt::fmt)
