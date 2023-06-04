//
// Created by Yoshiki on 2023/6/3.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_ARP_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_ARP_H_

#include "ip-echo.h"
#include <dpdk/rte_malloc.h>

#define ARP_HEADER_TOT_LEN (ETHER_HEADER_TOT_LEN + sizeof(struct rte_arp_hdr))

static inline void arpSend (
    struct rte_arp_hdr *arpHdr,
    uint16_t opcode,
    uint8_t *srcMac,
    uint8_t *dstMac,
    uint32_t srcIp,
    uint32_t dstIp
)
{

    // 值为1时表示以太网地址
    arpHdr->arp_hardware = htons(1);
    arpHdr->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
    // mac地址长度，为6
    arpHdr->arp_hlen = RTE_ETHER_ADDR_LEN;
    // ip地址长度，为4
    arpHdr->arp_plen = sizeof(uint32_t);

    arpHdr->arp_opcode = htons(opcode);
    rte_memcpy(arpHdr->arp_data.arp_sha.addr_bytes, srcMac, RTE_ETHER_ADDR_LEN);
    rte_memcpy(arpHdr->arp_data.arp_tha.addr_bytes, dstMac, RTE_ETHER_ADDR_LEN);

    arpHdr->arp_data.arp_sip = srcIp;
    arpHdr->arp_data.arp_tip = dstIp;
}
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_ARP_H_
