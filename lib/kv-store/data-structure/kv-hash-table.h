#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
//
// Created by 115282 on 2023/8/29.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_
#define LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_

#include <functional>
#include <cstddef>

#include <memory>
#include <ext/aligned_buffer.h>
#include <bits/hashtable_policy.h>

template <class _Key, class _Val, bool autoReserve = true, typename _Hash = std::hash <_Key>,
    typename _Pred = std::equal_to <_Key>,
    typename _Alloc = std::allocator <std::pair <const _Key, _Val>>>
class HashTable
{

    using HashNodeType = std::pair <const _Key, _Val>;
    struct PrimeRehashPolicy : std::__detail::_Prime_rehash_policy
    {
        // 缩容
        inline std::pair <bool, std::size_t> _M_need_scale_down (
            std::size_t __n_bkt,
            std::size_t __n_elt,
            std::size_t __n_des
        ) const
        {
            if (__n_elt < __n_des)
                return { false, 0 };
            // 已使用 / 总大小 < 0.1
            if ((__n_elt - __n_des) * 100 / __n_bkt < HASHTABLE_MIN_FILL)
            {
                double minBuckets =
                    ((__n_elt - __n_des) == 0 ? 11 : (__n_elt - __n_des) * 2) / _M_max_load_factor;

                return { true, _M_next_bkt(minBuckets) };
            }

            return { false, 0 };
        }

        static constexpr size_t HASHTABLE_MIN_FILL = 10;
    };
    struct _HashTableNodeNextPtr
    {
        _HashTableNodeNextPtr () = default;
        _HashTableNodeNextPtr *next = nullptr;
    };
    struct _HashTableNode : _HashTableNodeNextPtr
    {

        template <class ...Arg>
        explicit _HashTableNode (Arg &&...arg)
            : node(std::make_pair(std::forward <Arg>(arg)...)) {}
        HashNodeType node;
    };
    struct _HashTableNodeAllocator
    {
        using Alloc = typename _Alloc::template rebind <_HashTableNode *>::other;
        Alloc alloc;
    };

public:
    class HashTableIterator
    {
    public:
        explicit HashTableIterator (const _HashTableNode *node)
            : node(node) {}

        inline bool operator== (const HashTableIterator &rhs) const noexcept
        {
            return node == rhs.node;
        }
        inline bool operator!= (const HashTableIterator &rhs) const noexcept
        {
            return !operator==(rhs);
        }
        inline HashTableIterator &operator++ () noexcept
        {
            node = node->next;
            return *this;
        }
        inline HashTableIterator &operator++ (int) noexcept // NOLINT
        {
            auto old = *this;
            ++(*this);
            return old;
        }

    private:
        _HashTableNode *node;
    };
    enum class OperatorStatus
    {
        OPERATOR_SUCCESS,
        // 需要扩容，autoReserve为false时会返回
        FAILURE_NEED_EXPAND,
        // 需要缩容，autoReserve为false时会返回
        FAILURE_NEED_SCALE_DOWN,
        // KEY 重复
        FAILURE_KEY_REPEAT
    };

private:
    using Iterator = HashTable::HashTableIterator;
    using constIterator = const HashTable::HashTableIterator;

public:
    template <class ...Arg>
    std::pair <OperatorStatus, Iterator> emplace (Arg &&...arg)
    {

        auto doRehash = needRehash(1);
        if (doRehash.first)
        {
            if (autoReserve)
                reHash(doRehash.second);
            else
                return { OperatorStatus::FAILURE_NEED_EXPAND, Iterator { nullptr }};
        }

        emplacePrivate(std::forward <Arg>(arg)...);
    }

    inline void setMaxLoadFactor (float f)
    {
        primeRehashPolicy._M_max_load_factor = f;
    }

    // 判断操作是否需要扩容/缩容  > 0 扩容 < 0 缩容
    inline std::pair <bool, size_t> needRehash (int bkt) const
    {
        return bkt > 0
               ? primeRehashPolicy._M_need_rehash(bucketCount, elementCount, bkt)
               : primeRehashPolicy._M_need_scale_down(bucketCount, elementCount, -bkt);
    }

    inline Iterator begin () const noexcept
    {
        return { _begin };
    }

    inline Iterator end () const noexcept
    {
        return { nullptr };
    }

    inline Iterator begin () noexcept
    {
        return { _begin };
    }

    inline Iterator end () noexcept
    {
        return { nullptr };
    }

private:
    inline size_t getHashIndex (const _Key &key) const
    {
        return _Hash {}(key) % bucketCount;
    }
public:
    void reHash (size_t resize)
    {
        auto oldBuckets = buckets;
        buckets = hashTableNodeAllocator.alloc.allocate(resize);
        bucketCount = resize;

        for (auto v : *this)
        {

        }
    }

    inline void moveNode (_HashTableNode *tableNode)
    {
        auto hash = getHashIndex(tableNode->node.first);

        // 插入
    }

    template <class ...Arg>
    void emplacePrivate (Arg &&...arg)
    {

    }

private:
    size_t bucketCount = 0;
    size_t elementCount = 0;
    _HashTableNode **buckets;
    PrimeRehashPolicy primeRehashPolicy;
    _HashTableNodeNextPtr *_begin;

    _HashTableNodeAllocator hashTableNodeAllocator;
};
#endif //LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_

#pragma clang diagnostic pop