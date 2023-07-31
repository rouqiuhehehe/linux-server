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

#define LL_ADD(item, list) do {        \
    item->prev = NULL;                \
    item->next = list;                \
    if (list != NULL) list->prev = item; \
    list = item;                    \
} while(0)

#define LL_REMOVE(item, list) do {        \
    if (item->prev != NULL) item->prev->next = item->next;    \
    if (item->next != NULL) item->next->prev = item->prev;    \
    if (list == item) list = item->next;    \
    item->prev = item->next = NULL;            \
} while(0)

typedef struct
{
    struct rte_ring *in;
    struct rte_ring *out;
} InOutRing;
uint8_t gSrcMac[RTE_ETHER_ADDR_LEN];

InOutRing *getRingInstance ();

_Noreturn int pkgProcess (void *arg);
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_TCP_TCP_H_
