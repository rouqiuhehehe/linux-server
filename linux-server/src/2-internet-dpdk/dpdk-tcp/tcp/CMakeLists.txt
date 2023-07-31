aux_source_directory(. SOURCE_SRC)

set(CMAKE_C_FLAGS "-Wall -Wextra -g -O0 -msse4.1 -std=gnu11")

include_directories($ENV{RTE_SDK}/build/include)
link_directories($ENV{RTE_SDK}/build/lib)

set(DPDK_LIBS "-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive")

set(TARGET_NAME dpdk-tcp)
add_executable(${TARGET_NAME} ${SOURCE_SRC})
target_include_directories(${TARGET_NAME} PUBLIC "../")
target_link_libraries(${TARGET_NAME} ${DPDK_LIBS})
target_link_libraries(${TARGET_NAME} pthread dl rt m numa)