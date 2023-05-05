set(TARGET_NAME "${PROJECT_NAME}-flags")

include(RoqAutogen)

set(AUTOGEN_SCHEMAS flags.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/bybit/flags"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "hpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target(flags-autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT
  AUTOGEN_SOURCES
  NAMESPACE
  "roq/bybit/flags"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

roq_gitignore(OUTPUT ".gitignore" SOURCES "${TARGET_NAME}" ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library("${TARGET_NAME}" OBJECT ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" flags-autogen-headers)

target_link_libraries("${TARGET_NAME}" PRIVATE roq-api::roq-api absl::flags)
