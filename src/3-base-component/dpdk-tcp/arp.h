//
// Created by Administrator on 2023/5/4.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_ARP_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_ARP_H_

#define LL_ADD(item, list) do {        \
    (item)->prev = NULL;                \
    (item)->next = list;                \
    if ((list) != NULL) (list)->prev = item; \
    (list) = item;                    \
} while(0)

static uint8_t gDefaultArpMac[RTE_ETHER_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
struct arp_table
{

    struct arp_entry *entries;
    int count;
};

static struct arp_table *arpt = NULL;

struct arp_entry
{
    uint32_t ip;
    uint8_t hwaddr[RTE_ETHER_ADDR_LEN];

    uint8_t type;
    //

    struct arp_entry *next;
    struct arp_entry *prev;
};

static int ng_encode_arp_pkt (uint8_t *msg,
    uint16_t opcode,
    uint8_t *src_mac,
    uint8_t *dst_mac,
    uint32_t sip,
    uint32_t dip)
{

    // 1 ethhdr
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)msg;
    rte_memcpy(eth->s_addr.addr_bytes, src_mac, RTE_ETHER_ADDR_LEN);
    if (!strncmp((const char *)dst_mac, (const char *)gDefaultArpMac, RTE_ETHER_ADDR_LEN))
    {
        uint8_t mac[RTE_ETHER_ADDR_LEN] = { 0x0 };
        rte_memcpy(eth->d_addr.addr_bytes, mac, RTE_ETHER_ADDR_LEN);
    }
    else
    {
        rte_memcpy(eth->d_addr.addr_bytes, dst_mac, RTE_ETHER_ADDR_LEN);
    }
    eth->ether_type = htons(RTE_ETHER_TYPE_ARP);

    // 2 arp
    struct rte_arp_hdr *arp = (struct rte_arp_hdr *)(eth + 1);
    arp->arp_hardware = htons(1);
    arp->arp_protocol = htons(RTE_ETHER_TYPE_IPV4);
    arp->arp_hlen = RTE_ETHER_ADDR_LEN;
    arp->arp_plen = sizeof(uint32_t);
    arp->arp_opcode = htons(opcode);

    rte_memcpy(arp->arp_data.arp_sha.addr_bytes, src_mac, RTE_ETHER_ADDR_LEN);
    rte_memcpy(arp->arp_data.arp_tha.addr_bytes, dst_mac, RTE_ETHER_ADDR_LEN);

    arp->arp_data.arp_sip = sip;
    arp->arp_data.arp_tip = dip;

    return 0;

}
static struct rte_mbuf *ng_send_arp (struct rte_mempool *mbuf_pool,
    uint16_t opcode,
    uint8_t *src_mac,
    uint8_t *dst_mac,
    uint32_t sip,
    uint32_t dip)
{

    const unsigned total_length = sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr);

    struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
    if (!mbuf)
    {
        rte_exit(EXIT_FAILURE, "rte_pktmbuf_alloc\n");
    }

    mbuf->pkt_len = total_length;
    mbuf->data_len = total_length;

    uint8_t *pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t *);
    ng_encode_arp_pkt(pkt_data, opcode, src_mac, dst_mac, sip, dip);

    return mbuf;
}
static struct arp_table *arp_table_instance (void)
{

    if (arpt == NULL)
    {

        arpt = rte_malloc("arp table", sizeof(struct arp_table), 0);
        if (arpt == NULL)
        {
            rte_exit(EXIT_FAILURE, "rte_malloc arp table failed\n");
        }
        memset(arpt, 0, sizeof(struct arp_table));
    }

    return arpt;
}
static uint8_t *ng_get_dst_macaddr (uint32_t dip)
{

    struct arp_entry *iter;
    struct arp_table *table = arp_table_instance();

    for (iter = table->entries; iter != NULL; iter = iter->next)
    {
        if (dip == iter->ip)
        {
            return iter->hwaddr;
        }
    }

    return NULL;
}

static void
print_ethaddr (const char *name, const struct rte_ether_addr *eth_addr)
{
    char buf[RTE_ETHER_ADDR_FMT_SIZE];
    rte_ether_format_addr(buf, RTE_ETHER_ADDR_FMT_SIZE, eth_addr);
    printf("%s%s", name, buf);
}

static inline void arp_protocol (struct rte_mempool *mbuf_pool,
    struct rte_mbuf *mbuf,
    uint8_t *gSrcMac,
    struct inout_ring *ring,
    char *ip)
{
    struct rte_arp_hdr *ahdr = rte_pktmbuf_mtod_offset(mbuf,
                                                       struct rte_arp_hdr *, sizeof(struct rte_ether_hdr));

    struct in_addr addr;
    addr.s_addr = ahdr->arp_data.arp_tip;
    printf("arp ---> src: %s ", inet_ntoa(addr));

    inet_aton(ip, &addr);
    printf(" local: %s \n", inet_ntoa(addr));

    if (ahdr->arp_data.arp_tip == addr.s_addr)
    {

        if (ahdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REQUEST))
        {

            printf("arp --> request\n");

            struct rte_mbuf *arpbuf = ng_send_arp(
                mbuf_pool, RTE_ARP_OP_REPLY, gSrcMac, ahdr->arp_data.arp_sha.addr_bytes,
                ahdr->arp_data.arp_tip, ahdr->arp_data.arp_sip
            );

            //rte_eth_tx_burst(gDpdkPortId, 0, &arpbuf, 1);
            //rte_pktmbuf_free(arpbuf);

            rte_ring_mp_enqueue_burst(ring->out, (void **)&arpbuf, 1, NULL);

        }
        else if (ahdr->arp_opcode == rte_cpu_to_be_16(RTE_ARP_OP_REPLY))
        {

            printf("arp --> reply\n");

            struct arp_table *table = arp_table_instance();

            uint8_t *hwaddr = ng_get_dst_macaddr(ahdr->arp_data.arp_sip);
            if (hwaddr == NULL)
            {

                struct arp_entry *entry = rte_malloc("arp_entry", sizeof(struct arp_entry), 0);
                if (entry)
                {
                    memset(entry, 0, sizeof(struct arp_entry));

                    entry->ip = ahdr->arp_data.arp_sip;
                    rte_memcpy(entry->hwaddr, ahdr->arp_data.arp_sha.addr_bytes, RTE_ETHER_ADDR_LEN);
                    entry->type = 0;

                    LL_ADD(entry, table->entries);
                    table->count++;
                }

            }
#if ENABLE_DEBUG
            struct arp_entry *iter;
            for (iter = table->entries; iter != NULL; iter = iter->next)
            {

                struct in_addr addr;
                addr.s_addr = iter->ip;

                print_ethaddr("arp table --> mac: ", (struct rte_ether_addr *)iter->hwaddr);

                printf(" ip: %s \n", inet_ntoa(addr));

            }
#endif
            rte_pktmbuf_free(mbuf);
        }
    }
};
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_ARP_H_
