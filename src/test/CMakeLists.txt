file(GLOB SOURCE_SRC "*.c" "*.cpp")

message("${SOURCE_SRC} ------- ")

add_library(hook SHARED hook.c)
target_link_libraries(hook dl)

add_executable(main main.c)
target_link_libraries(main hook)

add_executable(test-cpp test.cpp)
target_link_libraries(test-cpp threadpool)

add_executable(cpp-thread-test cpp-thread-test.cpp)
target_link_libraries(cpp-thread-test pthread)

add_subdirectory(dpdk-test)