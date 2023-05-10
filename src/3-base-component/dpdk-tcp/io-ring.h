//
// Created by Administrator on 2023/5/4.
//

#ifndef LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_IO_RING_H_
#define LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_IO_RING_H_
#include <dpdk/rte_malloc.h>
#include <string.h>

#define RING_SIZE    1024

struct inout_ring
{
    struct rte_ring *in;
    struct rte_ring *out;
};
static struct inout_ring *rInst = NULL;
static struct inout_ring *ringInstance (void)
{

    if (rInst == NULL)
    {
        rInst = rte_malloc("in/out ring", sizeof(struct inout_ring), 0);
        memset(rInst, 0, sizeof(struct inout_ring));

        if (rInst == NULL)
        {
            rte_exit(EXIT_FAILURE, "ring buffer init failed\n");
        }

        if (rInst->in == NULL)
        {
            rInst->in = rte_ring_create("in ring", RING_SIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
        }
        if (rInst->out == NULL)
        {
            rInst->out = rte_ring_create("out ring", RING_SIZE, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
        }
    }

    return rInst;
}
#endif //LINUX_SERVER_SRC_3_BASE_COMPONENT_DPDK_TCP_IO_RING_H_
