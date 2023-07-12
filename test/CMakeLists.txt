set(TARGET_NAME ${PROJECT_NAME}-test)

set(SOURCES dummy.cpp main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME} roq-io::roq-io roq-logging::roq-logging aklomp::base64
                      Catch2::Catch2 ${RT_LIBRARIES})

set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)

# run

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
