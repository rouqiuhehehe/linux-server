//
// Created by Yoshiki on 2023/6/1.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_UDP_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_UDP_H_
#include "ip-echo.h"

#define UDP_HEADER_TOT_LEN ICMP_HEADER_TOT_LEN

static inline void udpSend (
    struct rte_udp_hdr *udpHdr,
    uint16_t srcPort,
    uint16_t dstPort,
    const uint8_t *data,
    uint32_t len,
    struct rte_ipv4_hdr *ipv4Hdr
)
{
    udpHdr->src_port = srcPort;
    udpHdr->dst_port = dstPort;
    udpHdr->dgram_len = htons(len + sizeof(struct rte_udp_hdr));
    rte_memcpy(udpHdr + 1, data, len);

    udpHdr->dgram_cksum = 0;
    udpHdr->dgram_cksum = rte_ipv4_udptcp_cksum(ipv4Hdr, udpHdr);
}
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_UDP_H_
