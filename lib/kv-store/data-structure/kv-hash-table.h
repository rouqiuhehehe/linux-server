#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
//
// Created by 115282 on 2023/8/29.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_
#define LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_

#include <iostream>
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
    using ExtractKey = std::__detail::_Select1st;
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
    struct _HashTableNodeHashCode
    {
        explicit _HashTableNodeHashCode (const _Key &key)
            : hashCode(_Hash {}(key)) {}
        const size_t hashCode;
    };
    struct _HashTableNodeBase
    {
        template <class ...Arg>
        explicit _HashTableNodeBase (Arg &&...arg)
            :
            node(std::make_pair(std::forward <Arg>(arg)...)) {}
        HashNodeType node;
    };
    struct _HashTableNode : _HashTableNodeBase, _HashTableNodeNextPtr, _HashTableNodeHashCode
    {

        template <class ...Arg>
        explicit _HashTableNode (Arg &&...arg)
            : _HashTableNodeBase(std::forward <Arg>(arg)...),
              _HashTableNodeHashCode(ExtractKey {}(this->node)) {}

        inline _HashTableNode *getNext () const noexcept
        {
            return static_cast<_HashTableNode *>(this->next);
        }
    };
    struct _HashTableNodeAllocator
    {
        using Alloc = typename _Alloc::template rebind <_HashTableNode>::other;
        Alloc alloc;
    };
    struct _HashTableBucketsAllocator
    {
        using Alloc = typename _Alloc::template rebind <_HashTableNodeNextPtr *>::other;
        Alloc alloc;
    };

    using NodePtr = _HashTableNode *;
    using NodeBasePtr = _HashTableNodeNextPtr *;

public:
    class HashTableIterator
    {
    public:
        explicit HashTableIterator (NodePtr node)
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
            node = node->getNext();
            return *this;
        }
        inline HashTableIterator &operator++ (int) noexcept // NOLINT
        {
            auto old = *this;
            ++(*this);
            return old;
        }

        inline HashNodeType &operator* () const noexcept
        {
            return node->node;
        }

        inline NodePtr operator-> () const noexcept
        {
            return node;
        }

        inline HashNodeType &operator* () noexcept
        {
            return node->node;
        }

        inline NodePtr operator-> () noexcept
        {
            return node;
        }

    private:
        NodePtr node;
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
    using ConstIterator = const HashTable::HashTableIterator;

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

        auto node = hashTableNodeAllocator.alloc.allocate(sizeof(_HashTableNode));
        hashTableNodeAllocator.alloc.construct(node, std::forward <Arg>(arg)...);

        const std::string &key = ExtractKey {}(node->node);
        size_t hashCode = getHash(key);
        size_t idx = getHashIndex(hashCode);

        auto p = findBeforeNode(idx, key, hashCode);
        if (p)
            return { OperatorStatus::FAILURE_KEY_REPEAT, Iterator(static_cast<NodePtr>(p->next)) };

        insertUniqueNode(idx, node);

        return { OperatorStatus::OPERATOR_SUCCESS, Iterator(node) };
    }

    size_t erase (const _Key &key)
    {
        size_t hashCode = getHash(key);
        size_t idx = getHashIndex(hashCode);

        auto p = findBeforeNode(idx, key, hashCode);
        if (!p) return 0;

        auto old = static_cast<NodePtr>(p->next);
        p->next = p->next->next;

        hashTableNodeAllocator.alloc.deallocate(old, 1);
        --elementCount;

        if (needRehash(-1))
        {
            
        }
        return 1;
    }
    std::pair <OperatorStatus, Iterator> erase (Iterator it)
    {
    }

    Iterator find (const _Key &key) const noexcept
    {
        size_t hashCode = getHash(key);
        size_t idx = getHashIndex(hashCode);

        auto p = findBeforeNode(idx, key, hashCode);
        if (p)
            return ConstIterator(static_cast<NodePtr>(p->next));

        return ConstIterator(nullptr);
    }

    Iterator find (const _Key &key) noexcept
    {
        size_t hashCode = getHash(key);
        size_t idx = getHashIndex(hashCode);

        auto p = findBeforeNode(idx, key, hashCode);
        if (p)
            return Iterator(static_cast<NodePtr>(p->next));

        return Iterator(p);
    }

    inline size_t size () const noexcept
    {
        return elementCount;
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
        return ConstIterator(static_cast<NodePtr>(_begin.next));
    }

    inline Iterator end () const noexcept
    {
        return ConstIterator(nullptr);
    }

    inline Iterator begin () noexcept
    {
        return Iterator(static_cast<NodePtr>(_begin.next));
    }

    inline Iterator end () noexcept
    {
        return Iterator(nullptr);
    }

public:
    inline size_t getHash (const _Key &key) const noexcept
    {
        return _Hash {}(key);
    }
    inline size_t getHash (const NodePtr p) const noexcept
    {
        return _Hash {}(ExtractKey {}(p->node));
    }
    inline size_t getHashIndex (const _Key &key) const noexcept
    {
        return getHash(key) % bucketCount;
    }

    inline size_t getHashIndex (const size_t hash) const noexcept
    {
        return hash % bucketCount;
    }

    inline size_t getHashIndex (const _HashTableNode &node) const
    {
        return node.hashCode % bucketCount;
    }

    Iterator insertUniqueNode (size_t idx, NodePtr node)
    {
        insertBucketBegin(idx, node);
        ++elementCount;
        return Iterator(node);
    }
    void reHash (size_t resize)
    {
        auto oldBuckets = buckets;
        buckets = bucketsAllocator.alloc.allocate(resize);
        bucketCount = resize;

        auto p = static_cast<NodePtr>(_begin.next);
        if (p)
        {
            _begin.next = nullptr;
            NodePtr next;
            do
            {
                next = p->getNext();
                moveNode(p);
                p = next;
            } while (p != nullptr);
        }
        bucketsAllocator.alloc.deallocate(oldBuckets, 1);
    }

    inline void moveNode (NodePtr tableNode)
    {
        auto idx = getHashIndex(*tableNode);

        // 插入
        insertBucketBegin(idx, tableNode);
    }

    void insertBucketBegin (size_t idx, NodePtr node)
    {
        // 如果当前slot已经有元素，头插
        if (buckets[idx])
        {
            node->next = buckets[idx]->next;
            buckets[idx]->next = node;
        }
        else
        {
            // 没有元素，直接修改_begin->next指针
            node->next = _begin.next;
            _begin.next = node;

            // hash slot储存的是单链表上一节点的数据
            // 如果node不是唯一节点，把获取node->next的slot，把slot指向node
            if (node->next)
                buckets[getHashIndex(*node->getNext())] = node;

            buckets[idx] = &_begin;
        }
    }

    NodeBasePtr findBeforeNode (size_t idx, const _Key &key, size_t hashCode) const noexcept
    {
        NodeBasePtr prevPtr = buckets[idx];
        if (!prevPtr)
            return nullptr;

        auto ptr = static_cast<NodePtr>(prevPtr->next);
        for (;; ptr = ptr->getNext())
        {
            if (equals(key, hashCode, ptr))
                return prevPtr;

            if (!ptr->next || getHashIndex(*ptr) != idx)
                break;

            prevPtr = ptr;
        }
        return nullptr;
    }

    bool equals (const _Key &key, const size_t hashCode, NodePtr node) const noexcept
    {
        static _Pred eq {};

        return node->hashCode == hashCode && eq(key, ExtractKey {}(node->node));
    }

private:
    size_t bucketCount = 0;
    size_t elementCount = 0;
    NodeBasePtr *buckets = nullptr;

    PrimeRehashPolicy primeRehashPolicy;
    _HashTableNodeNextPtr _begin;

    _HashTableNodeAllocator hashTableNodeAllocator;
    _HashTableBucketsAllocator bucketsAllocator;
};
#endif //LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_

#pragma clang diagnostic pop