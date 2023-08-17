executable()

target_link_libraries(main dl)

target_link_libraries(test-thread-pool threadpool)

target_link_libraries(cpp-thread-test pthread)

target_link_libraries(mysql-connect-pool-test mysqlconnectpool threadpool)
target_link_options(mysql-connect-pool-test PUBLIC -rdynamic)

target_link_directories(redis-test PUBLIC /usr/local/lib)
target_link_libraries(redis-test hiredis event)
target_include_directories(redis-test PUBLIC /usr/local/include)

add_subdirectory(dpdk-test)
add_subdirectory(cpp-hook)