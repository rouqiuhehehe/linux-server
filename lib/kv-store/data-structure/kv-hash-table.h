//
// Created by 115282 on 2023/8/29.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_
#define LINUX_SERVER_LIB_KV_STORE_DATA_STRUCTURE_KV_HASH_TABLE_H_

#include <iostream>
#include <functional>
#include <cstddef>

#include <memory>
#include <cstring>
#include <ext/aligned_buffer.h>
#include <bits/hashtable_policy.h>
#include "util/util.h"

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

    using NodePtr = _HashTableNode *;
    using NodeBasePtr = _HashTableNodeNextPtr *;

    struct _HashTableNodeAllocator
    {
        using Alloc = typename _Alloc::template rebind <_HashTableNode>::other;

        template <class ...Arg>
        inline NodePtr allocateNode (Arg &&...arg)
        {
            auto p = alloc.allocate(1);
            alloc.construct(p, std::forward <Arg>(arg)...);

            return p;
        }

        inline void deallocateNode (NodePtr ptr)
        {
            alloc.destroy(ptr);
            alloc.deallocate(ptr, 1);
        }

    private:
        Alloc alloc;
    };
    struct _HashTableBucketsAllocator
    {
        using Alloc = typename _Alloc::template rebind <_HashTableNodeNextPtr *>::other;

        inline NodeBasePtr *allocateBuckets (size_t n)
        {
            auto p = alloc.allocate(n);
            memset(p, 0, n * sizeof(NodeBasePtr)); // NOLINT

            return p;
        }

        inline void deallocateBuckets (NodeBasePtr *ptr, size_t n)
        {
            alloc.deallocate(ptr, n);
        }

    private:
        Alloc alloc;
    };

public:
    class _HashTableIterator : Utils::AllDefaultCopy
    {
    public:
        explicit _HashTableIterator (NodePtr node)
            : node(node) {}

        inline bool operator== (const _HashTableIterator &rhs) const noexcept
        {
            return node == rhs.node;
        }
        inline bool operator!= (const _HashTableIterator &rhs) const noexcept
        {
            return !operator==(rhs);
        }
        virtual inline _HashTableIterator &operator++ () noexcept
        {
            node = node->getNext();
            return *this;
        }
        virtual inline _HashTableIterator &operator++ (int) noexcept // NOLINT
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

        inline explicit operator bool () const noexcept
        {
            return node != nullptr;
        }

    protected:
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
        FAILURE_KEY_REPEAT,
        // KEY 未找到
        FAILURE_KEY_NOT_DEFINED
    };

public:
    using Iterator = HashTable::_HashTableIterator;
    using ConstIterator = const HashTable::_HashTableIterator;

public:
    HashTable () = default;
    ~HashTable () noexcept
    {
        clear();
    }

    HashTable (const HashTable &rhs)
    {
        operator=(rhs);
    }

    HashTable (HashTable &&rhs) noexcept
    {
        operator=(std::move(rhs));
    }

    HashTable &operator= (const HashTable &rhs)
    {
        if (&rhs == this)
            return *this;

        clear();
        buckets = bucketsAllocator.allocateBuckets(rhs.bucketCount);
        bucketCount = rhs.bucketCount;

        for (auto &v : rhs)
            emplace(v.first, v.second);

        return *this;
    }

    HashTable &operator= (HashTable &&rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        elementCount = rhs.elementCount;
        bucketCount = rhs.bucketCount;
        buckets = rhs.buckets;
        _begin = rhs._begin;

        rhs.elementCount = 0;
        rhs.bucketCount = 0;
        rhs.buckets = nullptr;
        rhs._begin.next = nullptr;

        return *this;
    }

    template <class ...Arg>
    std::pair <OperatorStatus, Iterator> emplace (Arg &&...arg)
    {

        auto status = checkNeedReHash(1);
        if (status == OperatorStatus::FAILURE_NEED_EXPAND && !autoReserve)
            return { status, end() };

        auto node = hashTableNodeAllocator.allocateNode(std::forward <Arg>(arg)...);

        const std::string &key = ExtractKey {}(node->node);
        size_t hashCode = getHash(key);
        size_t idx = getHashIndex(hashCode);

        auto p = findBeforeNode(idx, key, hashCode);
        if (p)
            return { OperatorStatus::FAILURE_KEY_REPEAT, Iterator(static_cast<NodePtr>(p->next)) };

        insertUniqueNode(idx, node);

        return { OperatorStatus::OPERATOR_SUCCESS, Iterator(node) };
    }

    std::pair <OperatorStatus, size_t> erase (const _Key &key)
    {
        if (empty())
            return { OperatorStatus::FAILURE_KEY_NOT_DEFINED, 0 };
        auto status = checkNeedReHash(-1);
        if (status == OperatorStatus::FAILURE_NEED_EXPAND && !autoReserve)
            return { status, 0 };

        size_t hashCode = getHash(key);
        size_t idx = getHashIndex(hashCode);

        auto p = findBeforeNode(idx, key, hashCode);
        if (!p) return { OperatorStatus::FAILURE_KEY_NOT_DEFINED, 0 };

        p->next = p->next->next;

        hashTableNodeAllocator.deallocateNode();
        --elementCount;

        return { status, 1 };
    }
    std::pair <OperatorStatus, Iterator> erase (Iterator it)
    {
        if (empty())
            return { OperatorStatus::FAILURE_KEY_NOT_DEFINED, end() };

        auto node = it.operator->();
        size_t idx = getHashIndex(*node);
        NodeBasePtr prevPtr = findBeforeNode(idx, node);

        size_t nextIdx = it->next == nullptr ? 0 : getHashIndex(*it->getNext());
        // 即 prevPtr->getNext() == it.node
        if (prevPtr == buckets[idx])
        {
            // 如果删除的节点为最后一个节点，或删除的节点next节点所在下标 != 删除节点的下标
            if (!prevPtr->next || nextIdx != idx)
                removeBucketBegin(idx, it->getNext(), nextIdx);
        }
        else if (it->next)
        {
            // 删除的节点next节点所在下标 != 删除节点的下标
            if (nextIdx != idx)
                // 修改buckets[nextIdx]指向删除节点的前一个节点
                buckets[nextIdx] = prevPtr;
        }

        prevPtr->next = it->next;
        hashTableNodeAllocator.deallocateNode(node);
        --elementCount;

        // 删除不做强制缩容
        auto status = checkNeedReHash(-1);
        return { status, Iterator(static_cast<NodePtr>(prevPtr->next)) };
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

    inline size_t empty () const noexcept
    {
        return elementCount == 0;
    }

    inline void clear () noexcept
    {
        if (!empty())
        {
            auto ptr = static_cast<NodePtr>(_begin.next);
            NodePtr old;
            do
            {
                old = ptr;
                ptr = ptr->getNext();
                hashTableNodeAllocator.deallocateNode(old);
            } while (ptr);
        }

        bucketsAllocator.deallocateBuckets(buckets, bucketCount);
        elementCount = 0;
        buckets = nullptr;
        bucketCount = 0;
        _begin.next = nullptr;
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

private:
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
        auto oldSize = bucketCount;

        buckets = bucketsAllocator.allocateBuckets(resize);
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
        bucketsAllocator.deallocateBuckets(oldBuckets, oldSize);
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

    void removeBucketBegin (size_t idx, NodePtr next, size_t nextIdx)
    {
        // 如果有next节点，将nextIdx储存的前节点换成当前节点
        if (next)
            buckets[nextIdx] = buckets[idx];

        // 如果当前节点是_begin，将_begin的next指向next节点
        if (buckets[idx] == &_begin)
            _begin.next = next;

        buckets[idx] = nullptr;
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

    NodeBasePtr findBeforeNode (size_t idx, NodePtr p) const noexcept
    {
        NodeBasePtr prevPtr = buckets[idx];

        while (prevPtr->next != p)
            prevPtr = prevPtr->next;

        return prevPtr;
    }

    bool equals (const _Key &key, const size_t hashCode, NodePtr node) const noexcept
    {
        static _Pred eq {};

        return node->hashCode == hashCode && eq(key, ExtractKey {}(node->node));
    }

    OperatorStatus checkNeedReHash (int bkt)
    {
        auto doReHash = needRehash(bkt);
        if (doReHash.first)
        {
            if (autoReserve)
                reHash(doReHash.second);

            return bkt > 0
                   ? OperatorStatus::FAILURE_NEED_EXPAND
                   : OperatorStatus::FAILURE_NEED_SCALE_DOWN;
        }

        return OperatorStatus::OPERATOR_SUCCESS;
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