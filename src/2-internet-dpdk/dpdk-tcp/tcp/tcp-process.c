//
// Created by Yoshiki on 2023/6/3.
//
#include "tcp.h"
#include "ip-echo.h"
#include "printf-color.h"
#include <dpdk/rte_errno.h>

#define TCP_INITIAL_WINDOW  14600
typedef enum NG_TCP_STATUS
{

    NG_TCP_STATUS_CLOSED,
    NG_TCP_STATUS_LISTEN,
    NG_TCP_STATUS_SYN_RCVD,
    NG_TCP_STATUS_SYN_SENT,
    NG_TCP_STATUS_ESTABLISHED,

    NG_TCP_STATUS_FIN_WAIT_1,
    NG_TCP_STATUS_FIN_WAIT_2,
    NG_TCP_STATUS_CLOSING,
    NG_TCP_STATUS_TIME_WAIT,

    NG_TCP_STATUS_CLOSE_WAIT,
    NG_TCP_STATUS_LAST_ACK

} TCP_STATUS;

typedef struct __TCB
{
    int fd;
    // 5元组
    uint32_t dip;
    uint16_t dport;
    uint32_t sip;
    uint16_t sport;

    uint8_t protocol;

    uint32_t seqNum;
    uint32_t ackNum;

    TCP_STATUS status;

    struct rte_ring *sendBuf;
    struct rte_ring *recvBuf;

    struct __TCB *next;
    struct __TCB *prev;

    pthread_cond_t cond;
    pthread_mutex_t mutex;
} TCB;

typedef struct
{
    int count;
    TCB *head;
} TcpTable;

typedef struct
{
    struct rte_tcp_hdr tcpHdr;

    uint32_t optLen;
    uint32_t option[10];
    uint8_t *data;
    uint32_t length;
} TcpFragment;

typedef struct __ArpEntry
{
    uint32_t ip;
    uint8_t addr[RTE_ETHER_ADDR_LEN];
    uint8_t type;

    struct __ArpEntry *next;
    struct __ArpEntry *prev;
} ArpEntry;

typedef struct
{
    ArpEntry *head;
    int count;

    pthread_spinlock_t spinlock;
} ArpTable;

static ArpTable *arpTable = NULL;

static TcpTable *getTcpTableInstance ()
{
    static TcpTable table = {};
    return &table;
}

static ArpTable *getArpTableInstance ()
{
    if (!arpTable)
    {
        arpTable = rte_malloc("arp table", sizeof(ArpTable), 0);
        memset(arpTable, 0, sizeof(ArpTable));
        pthread_spin_init(&arpTable->spinlock, PTHREAD_PROCESS_SHARED);
    }

    return arpTable;
}

static uint8_t *getDstMacAddr (uint32_t dip)
{
    ArpEntry *entry;
    ArpTable *table = getArpTableInstance();

    for (entry = table->head; entry; entry = entry->next)
    {
        if (dip == entry->ip)
            return entry->addr;
    }

    return NULL;
}

static int arpEntryInsert (uint32_t ip, uint8_t *mac)
{
    ArpTable *table = getArpTableInstance();

    uint8_t *addr = getDstMacAddr(ip);
    if (!addr)
    {
        ArpEntry *arpEntry = rte_malloc("arp entry", sizeof(ArpEntry), 0);

        if (arpEntry)
        {
            memset(arpEntry, 0, sizeof(ArpEntry));
            arpEntry->ip = ip;
            rte_memcpy(arpEntry->addr, mac, RTE_ETHER_ADDR_LEN);

            pthread_spin_lock(&arpTable->spinlock);
            LL_ADD(arpEntry, arpTable->head);
            arpTable->count++;
            pthread_spin_unlock(&arpTable->spinlock);

            return 0;
        }

        return -1;
    }

    return 1;
}

static uint32_t getRandomSeqNum ()
{
    uint32_t seed = time(NULL);
    return rand_r(&seed) % UINT32_MAX;
}

static TCB *tcpTableTCBSearch (uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport)
{
    TcpTable *tcpTable = getTcpTableInstance();
    TCB *iter;
    for (iter = tcpTable->head; iter != NULL; iter = iter->next)
    {
        if (iter->dip == dip && iter->dport == dport && iter->sip == sip && iter->sport == sport)
            return iter;
    }

    for (iter = tcpTable->head; iter != NULL; iter = iter->next)
    {
        if (iter->dport == dport && iter->sip == sip && iter->sport == sport)
            return iter;
    }

    return NULL;
}

static TCB *tcpTableTCBCreate (uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport)
{
    TCB *tcb = rte_malloc("tcb", sizeof(TCB), 0);
    if (!tcb)
        return NULL;

    tcb->sip = sip;
    tcb->dip = dip;
    tcb->sport = sport;
    tcb->dport = dport;
    tcb->protocol = IPPROTO_TCP;
    // unused
    tcb->fd = -1;
    //
    tcb->status = NG_TCP_STATUS_LISTEN;

    char bufName[32];
    sprintf(bufName, "recvBuf%x-%ud%d%c", sip, sport, getRandomSeqNum() % 255, '\0');
    tcb->recvBuf = rte_ring_create(bufName, RING_SIZE, (int)rte_socket_id(), 0);
    sprintf(bufName, "sendBuf%x-%ud%d%c", sip, sport, getRandomSeqNum() % 255, '\0');
    printf("%s\n", bufName);
    tcb->sendBuf = rte_ring_create(bufName, RING_SIZE, (int)rte_socket_id(), 0);

    tcb->seqNum = getRandomSeqNum();

    pthread_mutex_init(&tcb->mutex, NULL);
    pthread_cond_init(&tcb->cond, NULL);
    return tcb;
}

static int tcpCheckSum (struct rte_mbuf *mbuf)
{
    struct rte_ipv4_hdr *ipv4Hdr =
        rte_pktmbuf_mtod_offset(mbuf, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
    struct rte_tcp_hdr *tcpHdr = (struct rte_tcp_hdr *)(ipv4Hdr + 1);

    uint16_t tcpcksum = tcpHdr->cksum;
    tcpHdr->cksum = 0;
    uint16_t cksum = rte_ipv4_udptcp_cksum(ipv4Hdr, tcpHdr);

    if (cksum != tcpcksum)
    {
        printf("cksum : %x, tcpcksum : %x", cksum, tcpcksum);
        rte_pktmbuf_free(mbuf);
        return -1;
    }

    return 0;
}

static void tcpSendAckPkt (TCB *tcb, struct rte_tcp_hdr *tcpHdr)
{
    TcpFragment *tcpFragment = rte_malloc("tcp fragment", sizeof(TcpFragment), 0);
    if (!tcpFragment)
    {
        PRINTF_ERROR("rte_malloc tcpFragment fail\n");
        return;
    }
    memset(tcpFragment, 0, sizeof(TcpFragment));

    tcpFragment->tcpHdr.dst_port = tcpHdr->src_port;
    tcpFragment->tcpHdr.src_port = tcpHdr->dst_port;
    tcpFragment->tcpHdr.sent_seq = tcb->seqNum;
    tcpFragment->tcpHdr.recv_ack = tcb->ackNum;

    tcpFragment->tcpHdr.tcp_flags = RTE_TCP_ACK_FLAG;
    tcpFragment->tcpHdr.rx_win = TCP_INITIAL_WINDOW;
    tcpFragment->tcpHdr.data_off = 5 << 4;

    rte_ring_sp_enqueue(tcb->sendBuf, tcpFragment);
}

// 第一次握手，SYN=1，seqNum=x
static void tcpHandleFirstShakeHand (
    TCB *tcb,
    struct rte_ipv4_hdr *ipv4Hdr,
    struct rte_tcp_hdr *tcpHdr
)
{
    if (tcpHdr->tcp_flags & RTE_TCP_SYN_FLAG && tcb->status == NG_TCP_STATUS_LISTEN)
    {
        TcpTable *tcpTable = getTcpTableInstance();
        TCB *syn = tcpTableTCBCreate(
            ipv4Hdr->src_addr,
            ipv4Hdr->dst_addr,
            tcpHdr->src_port,
            tcpHdr->dst_port
        );
        LL_ADD(syn, tcpTable->head);
        TcpFragment *tcpFragment = rte_malloc("tcp fragment", sizeof(TcpFragment), 0);
        if (!tcpFragment)
        {
            perror("rte_malloc tcp fragment error");
            return;
        }
        memset(tcpFragment, 0, sizeof(TcpFragment));

        tcpFragment->tcpHdr.src_port = tcpHdr->dst_port;
        tcpFragment->tcpHdr.dst_port = tcpHdr->src_port;

        tcpFragment->tcpHdr.sent_seq = syn->seqNum;
        tcpFragment->tcpHdr.recv_ack = ntohl(tcpHdr->sent_seq) + 1;
        syn->ackNum = tcpFragment->tcpHdr.recv_ack;

        // 设置syn和ack标识位
        tcpFragment->tcpHdr.tcp_flags = RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG;
        tcpFragment->tcpHdr.rx_win = TCP_INITIAL_WINDOW;
        // 后4位保留位，前4位每位*4，5*4 为20头部长度
        tcpFragment->tcpHdr.data_off = 5 << 4;

        tcpFragment->data = NULL;
        tcpFragment->length = 0;

        rte_ring_sp_enqueue(syn->sendBuf, tcpFragment);
        syn->status = NG_TCP_STATUS_SYN_RCVD;
    }
}
// 第三次握手
static void tcpHandleThreeShakeHand (TCB *tcb, struct rte_tcp_hdr *tcpHdr)
{
    if (tcpHdr->tcp_flags & RTE_TCP_ACK_FLAG && tcb->status == NG_TCP_STATUS_SYN_RCVD)
    {
        uint32_t ackNum = ntohl(tcpHdr->recv_ack);

        if (ackNum != tcb->seqNum + 1)
        {
            printf("ackNum != seqNum + 1\n");
            return;
        }
        // 全连接状态
        tcb->status = NG_TCP_STATUS_ESTABLISHED;
        TCB *listener = tcpTableTCBSearch(0, 0, 0, tcb->dport);
        if (!listener)
            rte_exit(EXIT_FAILURE, "tcpTableTCBSearch error\n");

        pthread_mutex_lock(&listener->mutex);
        pthread_cond_signal(&listener->cond);
        pthread_mutex_unlock(&listener->mutex);
    }
}

static void tcpEnqueueRecvBuffer (TCB *tcb, struct rte_tcp_hdr *tcpHdr, uint32_t tcpLen)
{
    TcpFragment *tcpFragment = rte_malloc("tcp fragment", sizeof(TcpFragment), 0);
    if (!tcpFragment)
    {
        PRINTF_ERROR("rte_malloc tcpFragment error\n");
        return;
    }
    memset(tcpFragment, 0, sizeof(TcpFragment));

    tcpFragment->tcpHdr.src_port = ntohs(tcpHdr->src_port);
    tcpFragment->tcpHdr.dst_port = ntohs(tcpHdr->dst_port);

    // 有4位保留位
    uint8_t headerLen = (tcpHdr->data_off >> 4) * 4;
    uint32_t dataLen = tcpLen - headerLen;

    if (dataLen > 0)
    {
        uint8_t *data = (uint8_t *)tcpHdr + headerLen;
        tcpFragment->data = rte_malloc("tcp fragment data", dataLen + 1, 0);
        if (!tcpFragment->data)
        {
            PRINTF_ERROR("rte_malloc tcpFragment->data error\n");
            return;
        }
        memset(tcpFragment->data, 0, dataLen + 1);
        rte_memcpy(tcpFragment->data, data, dataLen);
        tcpFragment->length = dataLen;
    }

    rte_ring_sp_enqueue(tcb->recvBuf, tcpFragment);

    pthread_mutex_lock(&tcb->mutex);
    pthread_cond_signal(&tcb->cond);
    pthread_mutex_unlock(&tcb->mutex);
}

static void tcpHandleEstablished (TCB *tcb, struct rte_tcp_hdr *tcpHdr, uint32_t tcpLen)
{
    // tcp通信期
    if (!(tcpHdr->tcp_flags & RTE_TCP_FIN_FLAG))
    {
        tcpEnqueueRecvBuffer(tcb, tcpHdr, tcpLen);

        // 有4位保留位
        uint8_t headerLen = (tcpHdr->data_off >> 4) * 4;
        uint32_t dataLen = tcpLen - headerLen;
        // seq 和 ack 不包括54字节的头长
        tcb->ackNum = tcpHdr->sent_seq + dataLen;
        tcb->seqNum = ntohl(tcpHdr->recv_ack);

        tcpSendAckPkt(tcb, tcpHdr);
    }
        // 对端close期
    else
    {
        tcb->status = NG_TCP_STATUS_CLOSE_WAIT;
        tcpEnqueueRecvBuffer(tcb, tcpHdr, tcpHdr->data_off >> 4);
        tcb->ackNum = tcb->seqNum + 1;
        tcb->seqNum = ntohl(tcpHdr->recv_ack);

        tcpSendAckPkt(tcb, tcpHdr);
    }
}

static int tcpProcess (struct rte_mbuf *mbuf)
{
    if (tcpCheckSum(mbuf) != 0)
    {
        return -1;
    }

    struct rte_ipv4_hdr *ipv4Hdr =
        rte_pktmbuf_mtod_offset(mbuf, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
    struct rte_tcp_hdr *tcpHdr = (struct rte_tcp_hdr *)(ipv4Hdr + 1);

    TCB *tcb =
        tcpTableTCBSearch(ipv4Hdr->src_addr, ipv4Hdr->dst_addr, tcpHdr->src_port, tcpHdr->dst_port);

    if (!tcb)
    {
        // rte_pktmbuf_free(mbuf);
        // return -2;
        tcb = tcpTableTCBCreate(
            ipv4Hdr->src_addr,
            ipv4Hdr->dst_addr,
            tcpHdr->src_port,
            tcpHdr->dst_port
        );
    }

    uint32_t tcpLen = ntohs(ipv4Hdr->total_length) - sizeof(struct rte_ipv4_hdr);
    switch (tcb->status)
    {
        // client
        case NG_TCP_STATUS_CLOSED:
            break;
            // 第一次握手
        case NG_TCP_STATUS_LISTEN:
            tcpHandleFirstShakeHand(tcb, ipv4Hdr, tcpHdr);
            break;
        case NG_TCP_STATUS_SYN_RCVD:
            tcpHandleThreeShakeHand(tcb, tcpHdr);
            break;
            // client
        case NG_TCP_STATUS_SYN_SENT:
            break;
        case NG_TCP_STATUS_ESTABLISHED:
            tcpHandleEstablished(tcb, tcpHdr, tcpLen);
            break;
        case NG_TCP_STATUS_FIN_WAIT_1:
            break;
        case NG_TCP_STATUS_FIN_WAIT_2:
            break;
        case NG_TCP_STATUS_CLOSING:
            break;
        case NG_TCP_STATUS_TIME_WAIT:
            break;
        case NG_TCP_STATUS_CLOSE_WAIT:
            break;
        case NG_TCP_STATUS_LAST_ACK:
            break;
    }

    rte_pktmbuf_free(mbuf);

    return 0;
}

static void tcpEcho (
    struct rte_tcp_hdr *tcpHdr,
    TcpFragment *tcpFragment,
    struct rte_ipv4_hdr *ipv4Hdr
)
{
    tcpHdr->src_port = tcpFragment->tcpHdr.src_port;
    tcpHdr->dst_port = tcpFragment->tcpHdr.dst_port;
    tcpHdr->sent_seq = htonl(tcpFragment->tcpHdr.sent_seq);
    tcpHdr->recv_ack = htonl(tcpFragment->tcpHdr.recv_ack);

    tcpHdr->data_off = tcpFragment->tcpHdr.data_off;
    tcpHdr->rx_win = tcpFragment->tcpHdr.rx_win;
    tcpHdr->tcp_urp = tcpFragment->tcpHdr.tcp_urp;
    tcpHdr->tcp_flags = tcpFragment->tcpHdr.tcp_flags;

    if (tcpFragment->data)
    {
        // 数据的位置应该在tcp头+option选项后
        uint8_t *payload = (uint8_t *)(tcpHdr + 1) + tcpFragment->optLen * sizeof(uint32_t);
        rte_memcpy(payload, tcpFragment->data, tcpFragment->length);
    }

    tcpHdr->cksum = 0;
    tcpHdr->cksum = rte_ipv4_udptcp_cksum(ipv4Hdr, tcpHdr);
}
static void encodeTcpPkt (
    uint8_t *data,
    uint32_t sip,
    uint32_t dip,
    uint8_t *srcMac,
    uint8_t *dstMac,
    TcpFragment *tcpFragment
)
{
    uint32_t totLen =
        tcpFragment->length + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)
            + sizeof(struct rte_tcp_hdr) + tcpFragment->optLen * sizeof(uint32_t);

    struct rte_ether_hdr *etherHdr = (struct rte_ether_hdr *)data;
    struct rte_ipv4_hdr *ipv4Hdr = (struct rte_ipv4_hdr *)(etherHdr + 1);
    struct rte_tcp_hdr *tcpHdr = (struct rte_tcp_hdr *)(ipv4Hdr + 1);

    etherEcho(etherHdr, srcMac, dstMac, RTE_ETHER_TYPE_IPV4);
    ipEcho(ipv4Hdr, sip, dip, IPPROTO_TCP, totLen - sizeof(struct rte_ether_hdr));
    tcpEcho(tcpHdr, tcpFragment, ipv4Hdr);
}
static struct rte_mbuf *tcpPkt (
    struct rte_mempool *mempool,
    uint32_t sip,
    uint32_t dip,
    uint8_t *srcMac,
    uint8_t *dstMac,
    TcpFragment *tcpFragment
)
{
    uint32_t totLen =
        tcpFragment->length + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)
            + sizeof(struct rte_tcp_hdr) + tcpFragment->optLen * sizeof(uint32_t);

    struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mempool);
    if (!mbuf)
        rte_exit(EXIT_FAILURE, "tcpPkt rte_pktmbuf_alloc error\n");

    mbuf->data_len = totLen;
    mbuf->pkt_len = totLen;

    uint8_t *data = rte_pktmbuf_mtod(mbuf, uint8_t *);
    encodeTcpPkt(data, sip, dip, srcMac, dstMac, tcpFragment);

    return mbuf;
}
static void tcpOut (struct rte_mempool *mempool)
{
    TcpTable *tcpTable = getTcpTableInstance();

    TCB *tcb;
    TcpFragment *tcpFragment;
    uint32_t ret;
    for (tcb = tcpTable->head; tcb != NULL; tcb = tcb->next)
    {
        if (!tcb->sendBuf)
            continue;

        ret = rte_ring_sc_dequeue(tcb->sendBuf, (void **)&tcpFragment);
        if (ret != 0)
            continue;

        uint8_t *dstMac = getDstMacAddr(tcb->sip);
        if (dstMac)
        {
            struct rte_mbuf *mbuf = tcpPkt(
                mempool,
                tcb->sip,
                tcb->dip,
                gSrcMac,
                dstMac,
                tcpFragment
            );
            InOutRing *ring = getRingInstance();
            rte_ring_sp_enqueue_burst(ring->out, (void **)&mbuf, 1, NULL);

            if (tcpFragment->data)
                rte_free(tcpFragment->data);
            rte_free(tcpFragment);
        }

    }
}

_Noreturn int pkgProcess (void *arg)
{
    struct rte_mempool *mempool = (struct rte_mempool *)arg;
    InOutRing *ring = getRingInstance();
    struct rte_mbuf *mbufs[BURST_SIZE];
    uint32_t nums, i;

    while (1)
    {
        nums = rte_ring_sc_dequeue_burst(ring->in, (void **)mbufs, BURST_SIZE, NULL);

        for (i = 0; i < nums; ++i)
        {
            struct rte_ether_hdr *etherHdr = rte_pktmbuf_mtod(mbufs[i], struct rte_ether_hdr *);

            if (etherHdr->ether_type == htons(RTE_ETHER_TYPE_IPV4))
            {
                struct rte_ipv4_hdr *ipv4Hdr = (struct rte_ipv4_hdr *)(etherHdr + 1);

                // 保存mac地址
                arpEntryInsert(ipv4Hdr->src_addr, etherHdr->s_addr.addr_bytes);

                if (ipv4Hdr->next_proto_id == IPPROTO_TCP)
                {
                    tcpProcess(mbufs[i]);
                }
            }
        }
        tcpOut(mempool);
    }

}