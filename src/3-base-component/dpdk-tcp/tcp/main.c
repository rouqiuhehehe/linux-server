//
// Created by Yoshiki on 2023/6/4.
//

#include "tcp.h"

struct rte_mempool *initDpdk (int argc, char **argv, int dpdkPortId)
{
    int ret = rte_eal_init(argc, argv);
    CHECK_RET(ret < 0, "rte_eal_init error\n");

    // 内存池名字，mbuf的数量，一开始初始化多大（0默认），数据包能接收多大（0默认），每个mbuf数据缓冲区的大小，内存id
    struct rte_mempool *mempool = rte_pktmbuf_pool_create(
        "global memory pool",
        MEM_BUF_POOL_LEN,
        0,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        (int)rte_socket_id()
    );
    CHECK_RET(!mempool, "create the pktmbuf_pool error\n");

    ret = rte_eth_dev_count_avail();
    CHECK_RET(ret <= 0, "no network card available\n");

    // 获取网卡基本信息
    struct rte_eth_dev_info devInfo;
    rte_eth_dev_info_get(dpdkPortId, &devInfo);


    return mempool;
}
int main (int argc, char **argv)
{
    int dpdkPortId = 0;
    char srcMac[RTE_ETHER_ADDR_LEN] = {};

    initDpdk(argc, argv, dpdkPortId);

    return 0;
}