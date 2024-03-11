set(TARGET_NAME ${PROJECT_NAME}-flags)

include(RoqAutogen)

set(NAMESPACE "roq/pubsub/flags")

set(AUTOGEN_SCHEMAS flags.json)

if(BUILD_DOCS)

  roq_autodoc(
    OUTPUT
    AUTOGEN_DOC
    NAMESPACE
    ${NAMESPACE}
    OUTPUT_TYPE
    "doc"
    SOURCES
    ${AUTOGEN_SCHEMAS})

  add_custom_target(flags-autogen-doc ALL DEPENDS ${AUTOGEN_DOC})

  roq_allflags(OUTPUT "flags.rstinc" SOURCES ${AUTOGEN_DOC})

endif()

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  ${NAMESPACE}
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
  ${NAMESPACE}
  OUTPUT_TYPE
  "state"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

roq_gitignore(
  OUTPUT
  .gitignore
  SOURCES
  ${TARGET_NAME}
  ${AUTOGEN_DOC}
  ${AUTOGEN_HEADERS}
  ${AUTOGEN_SOURCES})

add_library(${TARGET_NAME} OBJECT ${AUTOGEN_SOURCES})

add_dependencies(${TARGET_NAME} flags-autogen-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-api::roq-api absl::flags)

# install

if(BUILD_DOCS)

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}/rst
    FILES_MATCHING
    PATTERN "*.rstinc"
    PATTERN "CMakeFiles" EXCLUDE)

endif()
