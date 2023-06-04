//
// Created by Yoshiki on 2023/6/4.
//
#include "tcp.h"

/**
    rte_ring_mp_enqueue_bulk         多生产者批量入队列，数量要求固定
    rte_ring_sp_enqueue_bulk         单生产者批量入队列，数量要求固定
    rte_ring_enqueue_bulk            生产者批量入队列，同时支持单生产和多生产，数量要求固定
    rte_ring_mp_enqueue              多生产者单次入队列
    rte_ring_sp_enqueue              单生产者单次入队列
    rte_ring_enqueue                 生产者单次入队列，同时支持单生产和多生产

    rte_ring_mc_dequeue_bulk         多消费者批量出队列，数量要求固定
    rte_ring_sc_dequeue_bulk         单消费者批量出队列，数量要求固定
    rte_ring_dequeue_bulk            消费者批量出队列，同时支持单消费和多消费，数量要求固定
    rte_ring_mc_dequeue              多消费者单次出队列
    rte_ring_sc_dequeue              单消费者单次出队列
    rte_ring_dequeue                 消费者单次出队列，同时支持单消费和多消费

    rte_ring_count                   已经加入的obj数量
    rte_ring_free_count              剩余容量
    rte_ring_full                    判断ring是否已经满了，没有容量了
    rte_ring_empty                   判断ring是否为空
    rte_ring_get_size                获取ring的大小
    rte_ring_get_capacity            获取ring的最大容量
    rte_ring_list_dump               打印系统所有ring的信息
    rte_ring_lookup                  根据名字查找ring是否存在
    rte_ring_mp_enqueue_burst        多生产者批量入队列，如果ring可用容量不够，加入最大能加入的数量
    rte_ring_sp_enqueue_burst        单生产者批量入队列，如果ring可用容量不够，加入最大能加入的数量
    rte_ring_enqueue_burst           生产者批量入队列，同时支持单生产和多生产，如果ring可用容量不够，加入最大能加入的数量
    rte_ring_mc_dequeue_burst        多消费者批量出队列，如果ring可用容量不够，加入最大能加入的数量
    rte_ring_sc_dequeue_burst        单消费者批量出队列，如果ring可用容量不够，加入最大能加入的数量
    rte_ring_dequeue_burst           消费者批量出队列，同时支持单消费和多消费，如果ring可用容量不够，加入最大能加入的数量
 */
static InOutRing *ring = NULL;

InOutRing *getRingInstance ()
{
    if (!ring)
    {
        ring = rte_malloc("in/out ring", sizeof(InOutRing), 0);
        // 单一生产者和单一消费者
        ring->in = rte_ring_create(
            "in ring",
            RING_SIZE,
            (int)rte_socket_id(),
            RING_F_SP_ENQ | RING_F_SC_DEQ
        );
        ring->out = rte_ring_create(
            "out ring",
            RING_SIZE,
            (int)rte_socket_id(),
            RING_F_SP_ENQ | RING_F_SC_DEQ
        );
    }

    return ring;
}