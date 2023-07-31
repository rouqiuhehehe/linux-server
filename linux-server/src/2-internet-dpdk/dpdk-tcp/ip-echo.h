//
// Created by Yoshiki on 2023/5/31.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_IP_ECHO_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_IP_ECHO_H_

#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_eal.h>

#define ETHER_HEADER_TOT_LEN sizeof(struct rte_ether_hdr)
#define IP_HEADER_TOT_LEN (ETHER_HEADER_TOT_LEN + sizeof(struct rte_ipv4_hdr))
#define ICMP_HEADER_TOT_LEN (IP_HEADER_TOT_LEN + sizeof(struct rte_icmp_hdr))

static uint16_t icmpCheckSum (uint16_t *icmpHdr, int count)
{
    register long sum = 0;

    while (count > 1)
    {

        sum += *(unsigned short *)icmpHdr++;
        count -= 2;

    }

    if (count > 0)
    {
        sum += *(unsigned char *)icmpHdr;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return ~sum;

}
static inline void etherEcho (
    struct rte_ether_hdr *etherHdr,
    uint8_t *srcMac,
    uint8_t *dstMac,
    int type
)
{
    rte_memcpy(etherHdr->s_addr.addr_bytes, srcMac, RTE_ETHER_ADDR_LEN);
    rte_memcpy(etherHdr->d_addr.addr_bytes, dstMac, RTE_ETHER_ADDR_LEN);
    etherHdr->ether_type = htons(type);
}

static inline void ipEcho (
    struct rte_ipv4_hdr *ipv4Hdr,
    uint32_t srcAddr,
    uint32_t dstAddr,
    uint8_t nextProto,
    uint16_t totLen
)
{
    // 0100 版本ipv4 0101 首部长度 5 * 4
    // 一个字节不需要转网络字节序
    ipv4Hdr->version_ihl = 0x45;
    // 0表示一般服务
    ipv4Hdr->type_of_service = 0;
    ipv4Hdr->total_length = htons(totLen);
    // 标识位，用于分片
    ipv4Hdr->packet_id = 0;
    ipv4Hdr->fragment_offset = 0;
    ipv4Hdr->time_to_live = 64;
    ipv4Hdr->next_proto_id = nextProto;
    ipv4Hdr->src_addr = srcAddr;
    ipv4Hdr->dst_addr = dstAddr;
    ipv4Hdr->hdr_checksum = 0;
    ipv4Hdr->hdr_checksum = rte_ipv4_cksum(ipv4Hdr);
}

static inline void icmpEcho (struct rte_icmp_hdr *icmpHdr, uint16_t id, uint16_t seq)
{
    icmpHdr->icmp_type = RTE_IP_ICMP_ECHO_REPLY;
    icmpHdr->icmp_code = 0;
    icmpHdr->icmp_ident = id;
    icmpHdr->icmp_seq_nb = seq;

    icmpHdr->icmp_cksum = 0;
    icmpHdr->icmp_cksum = icmpCheckSum((uint16_t *)icmpHdr, sizeof(struct rte_icmp_hdr));
}
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_IP_ECHO_H_
