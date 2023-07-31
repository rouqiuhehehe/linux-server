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
    struct rte_eth_conf defaultConf = {
        .rxmode = {
            .max_rx_pkt_len = RTE_ETHER_MAX_LEN
        }
    };
    rte_eth_dev_configure(dpdkPortId, DPDK_RX_NUM, DPDK_TX_NUM, &defaultConf);

    // 第几个网卡，第几个队列，分配多少个描述符的数量(不大于RTE_MBUF_DEFAULT_BUF_SIZE参数的数量就行)，id，配置，内存池
    ret = rte_eth_rx_queue_setup(
        dpdkPortId,
        0,
        RTE_MBUF_DEFAULT_DATAROOM,
        rte_eth_dev_socket_id(dpdkPortId),
        NULL,
        mempool
    );
    CHECK_RET(ret, "create rx queue error\n");

    struct rte_eth_txconf txConf = devInfo.default_txconf;
    txConf.offloads = defaultConf.rxmode.offloads;
    ret = rte_eth_tx_queue_setup(
        dpdkPortId,
        0,
        RTE_MBUF_DEFAULT_DATAROOM,
        rte_eth_dev_socket_id(dpdkPortId),
        &txConf
    );
    CHECK_RET(ret, "create tx queue error\n");

    ret = rte_eth_dev_start(dpdkPortId);
    CHECK_RET(ret, "start error\n");

    return mempool;
}

void mainLoop (int dpdkPortId)
{
    struct rte_mbuf *rx[BURST_SIZE], *tx[BURST_SIZE];
    InOutRing *ring = getRingInstance();
    uint32_t num, i;

    while (1)
    {
        num = rte_eth_rx_burst(dpdkPortId, 0, rx, BURST_SIZE);
        CHECK_RET(num > BURST_SIZE, "rte_eth_rx_burst error");
        if (num > 0)
            rte_ring_sp_enqueue_burst(ring->in, (void **)rx, num, NULL);

        // 取出将要发送的包
        num = rte_ring_sc_dequeue_burst(ring->out, (void **)tx, BURST_SIZE, NULL);
        CHECK_RET(num > BURST_SIZE, "rte_ring_sc_dequeue_burst error");
        if (num > 0)
        {
            rte_eth_tx_burst(dpdkPortId, 0, tx, num);
            for (i = 0; i < num; ++i)
                rte_pktmbuf_free(tx[i]);
        }

        usleep(10);
    }
}

int main (int argc, char **argv)
{
    setbuf(stdout, 0);

    int dpdkPortId = 0;

    struct rte_mempool *mempool = initDpdk(argc, argv, dpdkPortId);
    rte_eth_macaddr_get(dpdkPortId, (struct rte_ether_addr *)gSrcMac);

    uint32_t lcoreId = rte_lcore_id();
    lcoreId = rte_get_next_lcore(lcoreId, 1, 0);
    // 开线程处理
    rte_eal_remote_launch(pkgProcess, mempool, lcoreId);

    mainLoop(dpdkPortId);

    return 0;
}