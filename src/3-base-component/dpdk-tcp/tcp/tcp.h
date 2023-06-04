//
// Created by Yoshiki on 2023/6/3.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_TCP_TCP_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_TCP_TCP_H_
#include <dpdk/rte_eal.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_mbuf.h>
#include <dpdk/rte_malloc.h>

#define CHECK_RET(expr, msg) if(expr) rte_exit(EXIT_FAILURE, msg)
#define MEM_BUF_POOL_LEN 4096
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_TCP_TCP_H_
