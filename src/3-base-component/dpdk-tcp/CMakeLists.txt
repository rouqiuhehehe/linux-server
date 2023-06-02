project(dpdk-tcp)

SET(CMAKE_C_FLAGS "-Wall -Wextra -g -O0 -msse4.1 -std=gnu11")

INCLUDE_DIRECTORIES($ENV{RTE_SDK}/build/include)
LINK_DIRECTORIES($ENV{RTE_SDK}/build/lib)

SET(DPDK_LIBS "-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive")

ADD_EXECUTABLE(dpdk-tcp main.c ip-echo.h udp.h)
TARGET_LINK_LIBRARIES(dpdk-tcp ${DPDK_LIBS})
TARGET_LINK_LIBRARIES(dpdk-tcp pthread dl rt m numa)