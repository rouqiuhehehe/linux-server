#include <dpdk/rte_mbuf.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_eal.h>

#include <stdio.h>
#include <arpa/inet.h>

#define MBUF_COUNT (4*1024)
#define BUFFER_SIZE 32

int dpdkPortId = 0;
static const struct rte_eth_conf config = {
    .rxmode = {
        // 接收一个包最大的长度
        // 1500(MTU) + 14(etherHeader) + 4(check)
        .max_rx_pkt_len = RTE_ETHER_MAX_LEN
    }
};

int main (int argc, char **argv)
{
    setbuf(stdout, 0);
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "rte_eal_init fail");

    // 内存池名字，分配多少内存，一开始初始化多大（0默认），数据包能接收多大（0默认），每个mbuf数据缓冲区的大小，内存id
    struct rte_mempool *mBufPool = rte_pktmbuf_pool_create(
        "mBufPool",
        MBUF_COUNT,
        0,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        (int)rte_socket_id()
    );

    // 第几个网卡，网卡的接收队列数量，网卡的发送队列数量，网卡的配置
    rte_eth_dev_configure(dpdkPortId, 1, 0, (const struct rte_eth_conf *)&config);
    // 第几个网卡，第几个队列，分配多少个描述符的数量(不大于RTE_MBUF_DEFAULT_BUF_SIZE参数的数量就行)，id，配置，内存池
    rte_eth_rx_queue_setup(dpdkPortId, 0, 128, rte_eth_dev_socket_id(dpdkPortId), NULL, mBufPool);
    rte_eth_dev_start(dpdkPortId);

    while (1)
    {
        // 此处使用的内存池，不涉及到内存拷贝
        struct rte_mbuf *buffers[BUFFER_SIZE];
        unsigned num = rte_eth_rx_burst(dpdkPortId, 0, buffers, BUFFER_SIZE);

        unsigned i;
        for (i = 0; i < num; ++i)
        {
            struct rte_ether_hdr *etherHdr = rte_pktmbuf_mtod(buffers[i], struct rte_ether_hdr *);
            if (etherHdr->ether_type == htons(RTE_ETHER_TYPE_IPV4))
            {
                struct rte_ipv4_hdr *ipv4Hdr = rte_pktmbuf_mtod_offset(
                    buffers[i],
                    struct rte_ipv4_hdr *,
                    sizeof(struct rte_ether_hdr)
                );

                // 因为只有一个字节  所以不需要转
                if (ipv4Hdr->next_proto_id == IPPROTO_UDP)
                {
                    struct rte_udp_hdr *udpHdr = (struct rte_udp_hdr *)(ipv4Hdr + 1);
                    uint16_t len = ntohs(udpHdr->dgram_len);
                    *((char *)udpHdr + len) = '\0';

                    printf("data: %s\n", (char *)(udpHdr + 1));
                }
            }
        }
    }
}