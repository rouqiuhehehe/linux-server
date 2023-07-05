#include <dpdk/rte_mbuf.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_eal.h>

#include <stdio.h>
#include <arpa/inet.h>

#define ENABLED_ICMP
#define ENABLED_UDP
#define ENABLED_ARP

#if defined(ENABLED_UDP) || defined(ENABLED_ICMP) || defined(ENABLED_ARP)
#define ENABLED_SEND
#endif

#ifdef ENABLED_ICMP
#include "ip-echo.h"
#endif
#ifdef ENABLED_UDP
#include "udp.h"
#endif

#ifdef ENABLED_ARP
#include "arp.h"

#define HOST_IP "192.168.253.201"
#endif

#define MBUF_COUNT (4*1024)
#define BUFFER_SIZE 32

//192.168.231.80
static const struct rte_eth_conf config = {
    .rxmode = {
        // 接收一个包最大的长度
        // 1500(MTU) + 14(etherHeader) + 4(check)
        .max_rx_pkt_len = RTE_ETHER_MAX_LEN
    }
};
struct rte_mempool *initPort (int dpdkPortId)
{
    // 内存池名字，mbuf的数量，一开始初始化多大（0默认），数据包能接收多大（0默认），每个mbuf数据缓冲区的大小，内存id
    struct rte_mempool *mBufPool = rte_pktmbuf_pool_create(
        "mBufPool",
        MBUF_COUNT,
        0,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        (int)rte_socket_id()
    );
    int tx = 0, rx = 1;
#ifdef ENABLED_SEND
    tx = 1;
#endif
    // 第几个网卡，网卡的接收队列数量，网卡的发送队列数量，网卡的配置
    rte_eth_dev_configure(dpdkPortId, rx, tx, (const struct rte_eth_conf *)&config);
    // 第几个网卡，第几个队列，分配多少个描述符的数量(不大于RTE_MBUF_DEFAULT_BUF_SIZE参数的数量就行)，id，配置，内存池
    rte_eth_rx_queue_setup(dpdkPortId, 0, 1024, rte_eth_dev_socket_id(dpdkPortId), NULL, mBufPool);
#ifdef ENABLED_SEND
    rte_eth_tx_queue_setup(dpdkPortId, 0, 1024, rte_eth_dev_socket_id(dpdkPortId), NULL);
#endif
    rte_eth_dev_start(dpdkPortId);

    return mBufPool;
}

struct rte_mbuf *mBufAlloc (struct rte_mempool *mBufPool, const uint32_t totLen)
{
    struct rte_mbuf *buf = rte_pktmbuf_alloc(mBufPool);
    if (!buf)
        rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc\n");

    buf->data_len = totLen;
    buf->pkt_len = totLen;

    return buf;
}

#ifdef ENABLED_ICMP
static struct rte_mbuf *icmpEchoPkg (
    struct rte_mempool *mBufPool,
    uint8_t *srcMac,
    uint8_t *dstMac,
    uint32_t srcAddr,
    uint32_t dstAddr,
    uint16_t id,
    uint16_t seq
)
{
    struct rte_mbuf *buf = mBufAlloc(mBufPool, ICMP_HEADER_TOT_LEN);
    uint8_t *pktData = rte_pktmbuf_mtod(buf, uint8_t *);

    struct rte_ether_hdr *etherHdr = (struct rte_ether_hdr *)pktData;
    struct rte_ipv4_hdr *ipv4Hdr = (struct rte_ipv4_hdr *)(etherHdr + 1);
    struct rte_icmp_hdr *icmpHdr = (struct rte_icmp_hdr *)(ipv4Hdr + 1);

    etherEcho(etherHdr, srcMac, dstMac, RTE_ETHER_TYPE_IPV4);
    ipEcho(
        ipv4Hdr,
        srcAddr,
        dstAddr,
        IPPROTO_ICMP,
        ICMP_HEADER_TOT_LEN - ETHER_HEADER_TOT_LEN);
    icmpEcho(icmpHdr, id, seq);

    return buf;
}
#endif

#ifdef ENABLED_UDP
static struct rte_mbuf *udpEchoPkg (
    struct rte_mempool *mBufPool,
    uint8_t *srcMac,
    uint8_t *dstMac,
    uint32_t srcAddr,
    uint32_t dstAddr,
    uint16_t srcPort,
    uint16_t dstPort,
    const uint8_t *data,
    uint32_t len
)
{
    const uint32_t totLen = len + UDP_HEADER_TOT_LEN;
    struct rte_mbuf *buf = mBufAlloc(mBufPool, totLen);
    uint8_t *pktData = rte_pktmbuf_mtod(buf, uint8_t *);

    struct rte_ether_hdr *etherHdr = (struct rte_ether_hdr *)pktData;
    struct rte_ipv4_hdr *ipv4Hdr = (struct rte_ipv4_hdr *)(etherHdr + 1);
    struct rte_udp_hdr *udpHdr = (struct rte_udp_hdr *)(ipv4Hdr + 1);

    etherEcho(etherHdr, srcMac, dstMac, RTE_ETHER_TYPE_IPV4);
    ipEcho(
        ipv4Hdr,
        srcAddr,
        dstAddr,
        IPPROTO_UDP,
        totLen - ETHER_HEADER_TOT_LEN);
    udpSend(udpHdr, srcPort, dstPort, data, len, ipv4Hdr);

    return buf;
}
#endif

#ifdef ENABLED_ARP
static struct rte_mbuf *arpEchoSend (
    struct rte_mempool *mBufPool,
    uint16_t opcode,
    uint8_t *srcMac,
    uint8_t *dstMac,
    uint32_t srcIp,
    uint32_t dstIp
)
{
    const uint32_t totLen = ARP_HEADER_TOT_LEN;
    struct rte_mbuf *buf = mBufAlloc(mBufPool, totLen);
    uint8_t *pktData = rte_pktmbuf_mtod(buf, uint8_t *);

    struct rte_ether_hdr *etherHdr = (struct rte_ether_hdr *)pktData;
    struct rte_arp_hdr *arpHdr = (struct rte_arp_hdr *)(etherHdr + 1);

    etherEcho(etherHdr, srcMac, dstMac, RTE_ETHER_TYPE_ARP);
    arpSend(arpHdr, opcode, srcMac, dstMac, srcIp, dstIp);

    return buf;
}
#endif
// 192.168.253.201
int main (int argc, char **argv)
{
    setbuf(stdout, 0);
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "rte_eal_init fail");

    int dpdkPortId = 0;
    struct rte_mempool *mBufPool = initPort(dpdkPortId);

#ifdef ENABLED_ARP
    uint8_t srcMac[RTE_ETHER_ADDR_LEN] = {};
    rte_eth_macaddr_get(dpdkPortId, (struct rte_ether_addr *)srcMac);
#endif
    // 此处使用的内存池，不涉及到内存拷贝
    struct rte_mbuf *buffers[BUFFER_SIZE];
    printf("---------------\n");
    printf("%s\n", srcMac);
    while (1)
    {
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
                    ETHER_HEADER_TOT_LEN
                );

#ifdef ENABLED_UDP
                // 因为只有一个字节  所以不需要转
                if (ipv4Hdr->next_proto_id == IPPROTO_UDP)
                {
                    struct rte_udp_hdr *udpHdr = (struct rte_udp_hdr *)(ipv4Hdr + 1);
                    uint16_t len = ntohs(udpHdr->dgram_len);
                    *((char *)udpHdr + len) = '\0';

                    printf("data: %s\n", (char *)(udpHdr + 1));
                    struct rte_mbuf *buf = udpEchoPkg(
                        mBufPool,
                        etherHdr->d_addr.addr_bytes,
                        etherHdr->s_addr.addr_bytes,
                        ipv4Hdr->dst_addr,
                        ipv4Hdr->src_addr,
                        udpHdr->dst_port,
                        udpHdr->src_port,
                        (uint8_t *)(udpHdr + 1),
                        len - sizeof(struct rte_udp_hdr)
                    );

                    rte_eth_tx_burst(dpdkPortId, 0, &buf, 1);
                    rte_pktmbuf_free(buf);
                }
#endif
#ifdef ENABLED_ICMP
                else if (ipv4Hdr->next_proto_id == IPPROTO_ICMP)
                {
                    struct rte_icmp_hdr *icmpHdr = (struct rte_icmp_hdr *)(ipv4Hdr + 1);

                    if (icmpHdr->icmp_type == RTE_IP_ICMP_ECHO_REQUEST)
                    {
                        struct rte_mbuf *buf = icmpEchoPkg(
                            mBufPool,
                            etherHdr->d_addr.addr_bytes,
                            etherHdr->s_addr.addr_bytes,
                            ipv4Hdr->dst_addr,
                            ipv4Hdr->src_addr,
                            icmpHdr->icmp_ident,
                            icmpHdr->icmp_seq_nb
                        );

                        rte_eth_tx_burst(dpdkPortId, 0, &buf, 1);

                        rte_pktmbuf_free(buf);
                    }
                }
#endif
            }
#ifdef ENABLED_ARP
            else if (etherHdr->ether_type == htons(RTE_ETHER_TYPE_ARP))
            {
                struct rte_arp_hdr *arpHdr = rte_pktmbuf_mtod_offset(
                    buffers[i],
                    struct rte_arp_hdr *,
                    ETHER_HEADER_TOT_LEN
                );
                struct in_addr inAddr = {};
                inAddr.s_addr = arpHdr->arp_data.arp_tip;
                char *ip = inet_ntoa(inAddr);

                if (strcmp(HOST_IP, ip) == 0 && arpHdr->arp_opcode == htons(RTE_ARP_OP_REQUEST))
                {
                    struct rte_mbuf *buf = arpEchoSend(
                        mBufPool,
                        RTE_ARP_OP_REPLY,
                        srcMac,
                        arpHdr->arp_data.arp_sha.addr_bytes,
                        arpHdr->arp_data.arp_tip,
                        arpHdr->arp_data.arp_sip
                    );

                    rte_eth_tx_burst(dpdkPortId, 0, &buf, 1);
                    rte_pktmbuf_free(buf);
                }
            }
#endif

            rte_pktmbuf_free(buffers[i]);
        }
    }
}