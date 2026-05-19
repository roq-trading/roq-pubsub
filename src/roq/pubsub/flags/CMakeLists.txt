set(TARGET_NAME ${PROJECT_NAME}-flags)

include(RoqAutogen)

set(NAMESPACE "roq/pubsub/flags")

set(TARGET_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE})

set(SOURCES settings.cpp)

set(AUTOGEN_SCHEMAS flags.json)

if(BUILD_DOCS)

  roq_autogen_rst(
    OUTPUT
    AUTOGEN_DOCS
    SOURCES
    ${AUTOGEN_SCHEMAS}
    NAMESPACE
    ${NAMESPACE}
    TEMPLATE_DIR
    ${TEMPLATE_DIR}
    TEMPLATE_TYPE
    "doc")

  add_custom_target(flags-autogen-docs ALL DEPENDS ${AUTOGEN_DOCS})

endif()

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "state")

add_custom_target(flags-autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen_cpp(
  OUTPUT
  AUTOGEN_SOURCES
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "state")

roq_gitignore(
  OUTPUT
  .gitignore
  SOURCES
  ${TARGET_NAME}
  ${AUTOGEN_DOCS}
  ${AUTOGEN_HEADERS}
  ${AUTOGEN_SOURCES})

add_library(${TARGET_NAME} SHARED ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies(${TARGET_NAME} flags-autogen-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-api::roq-api roq-core::roq-core roq-flags::roq-flags roq-logging::roq-logging roq-server::roq-server-flags
                                             absl::flags)

target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${TARGET_NAME}" ROQ_BUILD_NUMBER="${ROQ_BUILD_NUMBER}")

install(TARGETS ${TARGET_NAME})

install(FILES ${AUTOGEN_HEADERS} DESTINATION ${TARGET_DIR})
