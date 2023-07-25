project(example)
set(CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/example)

executable()
target_link_libraries(coroutine-server coroutine)

target_include_directories(coroutine-mysql-client PUBLIC /usr/include/mysql)
target_link_libraries(coroutine-mysql-client coroutine mysqlclient)

target_link_libraries(redis-async-test PUBLIC redisasync)

target_link_libraries(redis-sync-test PUBLIC hiredis)