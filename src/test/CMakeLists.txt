file(GLOB SOURCE_SRC "*.c" "*.cpp")

add_library(hook SHARED hook.c)
target_link_libraries(hook dl)

add_executable(main main.c)
target_link_libraries(main hook)

add_executable(test-thread-pool test-thread-pool.cpp)
target_link_libraries(test-thread-pool threadpool)

add_executable(cpp-thread-test cpp-thread-test.cpp)
target_link_libraries(cpp-thread-test pthread)

add_executable(testcpp test.cpp)

add_executable(mysql-connect-pool-test mysql-connect-pool-test.cpp)
target_link_libraries(mysql-connect-pool-test mysqlconnectpool)

add_subdirectory(dpdk-test)