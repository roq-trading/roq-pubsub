set(TARGET_NAME "${PROJECT_NAME}-benchmark")

set(SOURCES create_order.cpp main.cpp)

roq_gitignore(OUTPUT ".gitignore" SOURCES "${TARGET_NAME}")

add_executable("${TARGET_NAME}" ${SOURCES})

target_link_libraries("${TARGET_NAME}" "${PROJECT_NAME}-json" "${PROJECT_NAME}-tools"
                      benchmark::benchmark)

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

install(TARGETS ${TARGET_NAME})
