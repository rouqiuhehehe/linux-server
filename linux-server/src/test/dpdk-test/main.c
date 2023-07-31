#include <dpdk/rte_eal.h>
#include <stdio.h>
#include <arpa/inet.h>

#define MAKE_IPV4_ADDR(a, b, c, d) ((a) + ((b)<<8) + ((c)<<16) + ((d)<<24))
static uint32_t ip = MAKE_IPV4_ADDR(192, 168, 0, 105);
#define HOST_IP "192.168.0.105"

int main (int argc, char **argv)
{
    setbuf(stdout, 0);
    printf("%d\n", ip);
    struct in_addr inAddr;

    inet_aton(HOST_IP, &inAddr);
    printf("%d\n", inAddr.s_addr);
    return 0;
    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL init\n");

    return 0;
}