project(cpp20test)
set(CMAKE_CXX_STANDARD 20)

executable()

target_include_directories(mysql-sub PRIVATE /usr/include/mysql)
target_link_libraries(mysql-sub mysqlclient)