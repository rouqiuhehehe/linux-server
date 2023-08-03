project(linux-server-lib)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_library(async async.c include/global.h include/util.h)
target_link_libraries(async pthread)

add_subdirectory(coroutine)
add_subdirectory(example)
add_subdirectory(thread-pool)
add_subdirectory(mysql)
add_subdirectory(redis-async)
add_subdirectory(kv-store)