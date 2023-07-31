set(LIBRARY_NAME mysqlconnectpool)

aux_source_directory(. SOURCE_SRC)
add_library(${LIBRARY_NAME} ${SOURCE_SRC})

target_link_libraries(${LIBRARY_NAME} PUBLIC mysqlclient pthread)
target_include_directories(${LIBRARY_NAME} PUBLIC /usr/include/mysql)