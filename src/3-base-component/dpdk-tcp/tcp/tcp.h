//
// Created by Yoshiki on 2023/6/3.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_TCP_TCP_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_TCP_TCP_H_
#include <dpdk/rte_eal.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_mbuf.h>
#include <dpdk/rte_malloc.h>

#include <unistd.h>

#define MEM_BUF_POOL_LEN (4096 - 1)
#define RING_SIZE 1024
#define DPDK_RX_NUM 1
#define DPDK_TX_NUM 1
#define BURST_SIZE 32

#define CHECK_RET(expr, msg) if(expr) rte_exit(EXIT_FAILURE, "%s:%s:%d"msg, __FILE__, __FUNCTION__, __LINE__)

typedef struct
{
    struct rte_ring *in;
    struct rte_ring *out;
} InOutRing;

InOutRing *getRingInstance ();

int tcpProcess (void *arg);
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_TCP_TCP_H_
