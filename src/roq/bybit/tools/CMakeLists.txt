set(TARGET_NAME "${PROJECT_NAME}-tools")

set(SOURCES crypto.cpp)

add_library("${TARGET_NAME}" OBJECT ${SOURCES})

include(RoqAutogen)

set(AUTOGEN_SCHEMAS api.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/bybit/tools"
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "hpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_custom_target("${TARGET_NAME}-autogen-headers" ALL DEPENDS ${AUTOGEN_HEADERS})

roq_gitignore(OUTPUT ".gitignore" SOURCES "${TARGET_NAME}" ${AUTOGEN_HEADERS})

add_dependencies("${TARGET_NAME}" "${TARGET_NAME}-autogen-headers")

target_link_libraries("${TARGET_NAME}" roq-core::roq-core fmt::fmt)
