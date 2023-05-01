//
// Created by Administrator on 2023/4/4.
//
#define NETMAP_WITH_LIBS

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <net/netmap_user.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/icmp.h>

#define ETHER_IP_LEN 6
#define BYTE_ONE unsigned char
#define BYTE_TWO unsigned short
#define BYTE_FOUR unsigned int
#define LOCALHOST "192.168.0.102"
#define HMAC "00:0c:29:4e:c7:a6"

#pragma pack(1)
struct EtherHeader
{
    BYTE_ONE targetIP[ETHER_IP_LEN];
    BYTE_ONE sourceIP[ETHER_IP_LEN];
    BYTE_TWO type;
};

struct IPHeader
{
    BYTE_ONE version: 4,
        headerLen: 4;

    BYTE_ONE TOS;
    BYTE_TWO totLen;
    BYTE_TWO flag;
    BYTE_TWO sign: 3,
        offset: 13;

    BYTE_ONE ttl;
    BYTE_ONE protocol;
    BYTE_TWO check;

    BYTE_FOUR targetIP;
    BYTE_FOUR sourceIP;
};

struct UdpHeader
{
    BYTE_TWO sourcePort;
    BYTE_TWO targetPort;
    BYTE_TWO len;
    BYTE_TWO check;
};

struct UdpPackage
{
    struct EtherHeader etherHeader; //14
    struct IPHeader ipHeader; // 20
    struct UdpHeader udpHeader; // 8
    // 零长数组
    BYTE_ONE data[0];
};

struct ArpHeader
{
    unsigned short hType;
    unsigned short hProto;
    unsigned char hAddrLen;
    unsigned char protoLen;
    unsigned short oper;
    unsigned char sMac[ETHER_IP_LEN];
    unsigned int sip;
    unsigned char dMac[ETHER_IP_LEN];
    unsigned int dip;
};
// 将ip地址解析成mac地址
struct ArpPackage
{
    struct EtherHeader etherHeader;
    struct ArpHeader arpHeader;
};

struct IcmpHeader
{
    unsigned char type;
    unsigned char code;
    unsigned short check;
    unsigned short identifier;
    unsigned short seq;
    unsigned char data[32];
};

// ping命令协议
struct IcmpPackage
{
    struct EtherHeader etherHeader;
    struct IPHeader ipHeader;
    struct IcmpHeader icmpHeader;
};
#pragma pack()

void echoUdpPackage (struct UdpPackage *desc, struct UdpPackage *src)
{
    memcpy(desc, src, sizeof(struct UdpPackage));
    memcpy(desc->etherHeader.targetIP, src->etherHeader.sourceIP, ETHER_IP_LEN);
    memcpy(desc->etherHeader.sourceIP, src->etherHeader.targetIP, ETHER_IP_LEN);

    desc->ipHeader.sourceIP = src->ipHeader.targetIP;
    desc->ipHeader.targetIP = src->ipHeader.sourceIP;

    desc->udpHeader.sourcePort = src->udpHeader.targetPort;
    desc->udpHeader.targetPort = src->udpHeader.sourcePort;

}

static unsigned short checkCode (unsigned short *addr, int len)
{
    register int nleft = len;
    register unsigned short *w = addr;
    register int sum = 0;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(u_char *)(&answer) = *(u_char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return (answer);

}
void echoIcmpPackage (struct IcmpPackage *desc, struct IcmpPackage *src)
{
    memcpy(desc, src, sizeof(struct IcmpPackage));

    desc->icmpHeader.type = 0;
    desc->icmpHeader.code = 0;
    desc->icmpHeader.check = 0;

    desc->ipHeader.targetIP = src->ipHeader.sourceIP;
    desc->ipHeader.sourceIP = src->ipHeader.targetIP;

    memcpy(desc->etherHeader.sourceIP, src->etherHeader.targetIP, ETHER_IP_LEN);
    memcpy(desc->etherHeader.targetIP, src->etherHeader.sourceIP, ETHER_IP_LEN);

    desc->icmpHeader.check = checkCode((unsigned short *)&desc->icmpHeader,
                                       sizeof(struct IcmpHeader));
}

static int str2mac (unsigned char *mac, char *str)
{

    char *p = str;
    unsigned char value = 0x0;
    int i = 0;

    while (p != '\0')
    {

        if (*p == ':')
        {
            mac[i++] = value;
            value = 0x0;
        }
        else
        {

            unsigned char temp = *p;
            if (temp <= '9' && temp >= '0')
            {
                temp -= '0';
            }
            else if (temp <= 'f' && temp >= 'a')
            {
                temp -= 'a';
                temp += 10;
            }
            else if (temp <= 'F' && temp >= 'A')
            {
                temp -= 'A';
                temp += 10;
            }
            else
            {
                break;
            }
            value <<= 4;
            value |= temp;
        }
        p++;
    }

    mac[i] = value;

    return 0;
}

void echoArpPackage (struct ArpPackage *desc,
    struct ArpPackage *src,
    char *hmac)
{
    memcpy(desc, src, sizeof(struct ArpPackage));
    memcpy(desc->etherHeader.targetIP, src->etherHeader.sourceIP, ETHER_IP_LEN);

    str2mac(desc->etherHeader.sourceIP, hmac);

    desc->arpHeader.hAddrLen = 6;
    desc->arpHeader.protoLen = 4;
    desc->arpHeader.oper = htons(2);

    str2mac(desc->arpHeader.sMac, hmac);
    desc->arpHeader.sip = src->arpHeader.dip;

    memcpy(desc->arpHeader.dMac, src->arpHeader.sMac, ETHER_IP_LEN);
    desc->arpHeader.dip = src->arpHeader.sip;
}

int main ()
{
    setbuf(stdout, 0);
    struct epoll_event event, epollEvents[1024];
    unsigned char *stream = NULL;
    struct nm_pkthdr nmHeader = {};
    struct EtherHeader *etherHeader = NULL;

    int epfd = epoll_create(1024);
    struct nm_desc *nm = nm_open("netmap:ens38", NULL, 0, NULL);
    event.data.fd = nm->fd;
    event.events = EPOLLIN;

    epoll_ctl(epfd, EPOLL_CTL_ADD, nm->fd, &event);

    int nready;
    while (1)
    {
        nready = epoll_wait(epfd, epollEvents, 1024, -1);
        if (nready < 0)
            continue;

        if (epollEvents[0].events & EPOLLIN)
        {
            stream = nm_nextpkt(nm, &nmHeader);
            if (stream == NULL)
                continue;
            etherHeader = (struct EtherHeader *)stream;
            // 如果是ip协议
            if (ntohs(etherHeader->type) == ETHERTYPE_IP)
            {
                struct UdpPackage *udp = (struct UdpPackage *)stream;
                // 如果是udp协议
                if (udp->ipHeader.protocol == IPPROTO_UDP)
                {
                    struct in_addr addr;
                    addr.s_addr = udp->ipHeader.targetIP;

                    int udpLen = ntohs(udp->udpHeader.len);
                    printf(
                        "%s:%d, length: %d, ipLen: %d \n",
                        inet_ntoa(addr),
                        ntohs(udp->udpHeader.sourcePort),
                        udpLen,
                        ntohs(udp->ipHeader.totLen)
                    );

                    udp->data[udpLen - 8] = '\0';
                    printf("udp message : %s\n", udp->data);
                    struct UdpPackage echoUdp;
                    echoUdpPackage(&echoUdp, udp);
                    // 回包
                    nm_inject(nm, &echoUdp, sizeof(struct UdpPackage) + strlen(echoUdp.data));
                }
                else if (udp->ipHeader.protocol == IPPROTO_ICMP)
                {
                    struct IcmpPackage *icmp = (struct IcmpPackage *)stream;
                    printf(
                        "icmp --- %d，%x\n",
                        icmp->icmpHeader.type,
                        icmp->icmpHeader.check
                    );
                    if (icmp->icmpHeader.type == ICMP_ECHO)
                    {
                        struct IcmpPackage echoIcmp = {};
                        echoIcmpPackage(&echoIcmp, icmp);
                        nm_inject(nm, &echoIcmp, sizeof(struct IcmpPackage));
                    }
                    else
                        printf("other ip package ------\n");
                }
            }
            else if (ntohs(etherHeader->type) == ETHERTYPE_ARP)
            {
                struct ArpPackage *arpPackage = (struct ArpPackage *)stream;
                struct ArpPackage echoArp = {};

                struct in_addr inAddr = {};
                inAddr.s_addr = arpPackage->arpHeader.sip;
                if (arpPackage->arpHeader.dip == inet_addr(LOCALHOST))
                {
                    printf("arp ---- from %s\n", inet_ntoa(inAddr));
                    echoArpPackage(&echoArp, arpPackage, HMAC);
                    nm_inject(nm, &echoArp, sizeof(struct ArpPackage));
                }
            }
        }
    }
    return 0;
}