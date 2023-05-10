project(dpdk_hello)

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_FLAGS "-Wall -Wextra -g -O3 -msse4.1 -std=gnu11")

INCLUDE_DIRECTORIES($ENV{RTE_SDK}/build/include)
LINK_DIRECTORIES($ENV{RTE_SDK}/build/lib)

SET(DPDK_LIBS "-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive")

ADD_EXECUTABLE(dpdk_hello main.c)
TARGET_LINK_LIBRARIES(dpdk_hello ${DPDK_LIBS})
TARGET_LINK_LIBRARIES(dpdk_hello pthread dl rt m numa)