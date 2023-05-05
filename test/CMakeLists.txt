set(TARGET_NAME "${PROJECT_NAME}-test")

set(SOURCES
    json_account_info.cpp
    json_amend_order.cpp
    json_auth.cpp
    json_cancel_all_orders.cpp
    json_cancel_order.cpp
    json_error.cpp
    json_execution.cpp
    json_instrument_info.cpp
    json_open_orders.cpp
    json_order_book.cpp
    json_order.cpp
    json_ping.cpp
    json_place_order.cpp
    json_position.cpp
    json_position_info.cpp
    json_public_trade.cpp
    json_subscribe.cpp
    json_tickers.cpp
    json_wallet_balance.cpp
    json_wallet.cpp
    main.cpp)

roq_gitignore(OUTPUT ".gitignore" SOURCES "${TARGET_NAME}")

add_executable("${TARGET_NAME}" ${SOURCES})

target_link_libraries(
  "${TARGET_NAME}"
  "${PROJECT_NAME}-json"
  roq-io::roq-io
  roq-logging::roq-logging
  aklomp::base64
  Catch2::Catch2
  ${RT_LIBRARIES})

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

# run

add_test(NAME "${TARGET_NAME}" COMMAND "${TARGET_NAME}")
