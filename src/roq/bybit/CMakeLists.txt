set(TARGET_NAME "${PROJECT_NAME}")

add_subdirectory(flags)

set(SOURCES
    account.cpp
    application.cpp
    config.cpp
    drop_copy.cpp
    flags.cpp
    gateway.cpp
    market_data.cpp
    order_entry.cpp
    rest.cpp
    shared.cpp
    main.cpp)

add_subdirectory("json")
add_subdirectory("tools")

include(RoqAutogen)

set(AUTOGEN_SCHEMAS order_entry_state.json rest_state.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/bybit"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "hpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target(autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_gitignore(OUTPUT ".gitignore" SOURCES "${TARGET_NAME}" ${AUTOGEN_HEADERS})

add_executable("${TARGET_NAME}" ${SOURCES})

add_dependencies("${TARGET_NAME}" autogen-headers autogen-json-headers)

target_link_libraries(
  "${TARGET_NAME}"
  PRIVATE "${PROJECT_NAME}-json"
          "${PROJECT_NAME}-tools"
          "${PROJECT_NAME}-flags"
          roq-server::roq-server
          roq-web::roq-web
          roq-io::roq-io
          roq-core::roq-core
          roq-logging::roq-logging
          roq-api::roq-api
          fmt::fmt
          absl::flags
          absl::flat_hash_map
          absl::flat_hash_set
          aklomp::base64
          ${UUID_LIBRARIES}
          ${RT_LIBRARIES})

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

target_compile_definitions(
  "${TARGET_NAME}"
  PRIVATE ROQ_PACKAGE_NAME="${TARGET_NAME}" ROQ_HOST="${ROQ_HOST}"
          ROQ_BUILD_VERSION="${GIT_REPO_VERSION}" ROQ_GIT_DESCRIBE_HASH="${GIT_DESCRIBE_HASH}"
          ROQ_BUILD_NUMBER="${ROQ_BUILD_NUMBER}" ROQ_BUILD_TYPE="${ROQ_BUILD_TYPE}")

install(TARGETS ${TARGET_NAME})
