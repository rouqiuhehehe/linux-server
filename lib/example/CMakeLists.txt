project(example)
set(CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/example)

add_executable(coroutine-server coroutine-server.c)
target_link_libraries(coroutine-server coroutine)

add_executable(coroutine-mysql-client coroutine-mysql-client.c)
target_include_directories(coroutine-mysql-client PUBLIC /usr/include/mysql)
target_link_libraries(coroutine-mysql-client coroutine mysqlclient)

add_executable(bloom-filter-test bloom-filter-test.cpp)