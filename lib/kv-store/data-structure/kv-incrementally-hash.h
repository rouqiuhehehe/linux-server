//
// Created by 115282 on 2023/8/28.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_

#include "kv-hash-table.h"
#include "printf-color.h"

template <class _Key, class _Val, typename _Hash = std::hash <_Key>,
    typename _Pred = std::equal_to <_Key>,
    typename _Alloc = std::allocator <std::pair <const _Key, _Val>>>
class IncrementallyHashTable
{
    using _HashTable = HashTable <_Key, _Val, false, _Hash, _Pred, _Alloc>;

public:
    enum class ReHashStatus
    {
        DEFAULT,
        RE_HASHING_EXPAND,
        RE_HASHING_SCALE_DOWN
    };

    class _IncrementallyHashTableIterator : public _HashTable::Iterator
    {
        friend IncrementallyHashTable;
    public:
        using _HashTable::_HashTableIterator::_HashTableIterator;
        _IncrementallyHashTableIterator (const typename _HashTable::Iterator &rhs) // NOLINT
            : _HashTable::Iterator(rhs) {}
    };

public:
    using Iterator = _IncrementallyHashTableIterator;
    using ConstIterator = const _IncrementallyHashTableIterator;

    IncrementallyHashTable ()
    {
        // 先进行一次扩容，HashTable实现默认是不进行第一次扩容的
        h1.reHash(h1.needRehash(11).second);
    }
    ~IncrementallyHashTable () noexcept
    {
        if (status == ReHashStatus::DEFAULT)
            h1.clear();
        else
        {
            h1.elementCount = 0;
            h2.clear();
        }
    };

    IncrementallyHashTable (const IncrementallyHashTable &rhs)
    {
        this->operator=(rhs);
    }

    IncrementallyHashTable &operator= (const IncrementallyHashTable &rhs)
    {
        if (this == &rhs)
            return *this;

        h1(rhs.h1);
        h2(rhs.h2);
        status = rhs.status;

        typename _HashTable::NodeBasePtr node;
        if (rhs.rehashMoveNextNode)
        {
            size_t idx = rhs.h1.getHash(*rhs.rehashMoveNextNode);
            node = h1.findBeforeNode(
                idx,
                rhs.rehashMoveNextNode->node.first,
                rhs.rehashMoveNextNode->hashCode
            );
            if (node)
                rehashMoveNextNode = static_cast<typename _HashTable::NodePtr>(node)->getNext();
        }

        if (rhs.rehashMovePrevNode)
        {
            if (rhs.rehashMovePrevNode == &rhs.h1._begin)
                rehashMovePrevNode = &h1._begin;
            else
            {
                size_t idx = rhs.h1.getHash(*rhs.rehashMovePrevNode);
                node = h1.findBeforeNode(
                    idx,
                    rhs.rehashMovePrevNode->node.first,
                    rhs.rehashMovePrevNode->hashCode
                );
                if (node)
                    rehashMovePrevNode = node->next;
            }
        }

    }

    template <class ...Arg>
    std::pair <bool, Iterator> emplace (Arg &&...arg)
    {
        moveOneNode();
        if (status == ReHashStatus::DEFAULT)
        {
            auto res = h1.emplace(std::forward <Arg>(arg)...);
            if (res.first != _HashTable::OperatorStatus::OPERATOR_SUCCESS)
            {
                if (res.first == _HashTable::OperatorStatus::FAILURE_KEY_REPEAT)
                    return { false, end() };
                else if (res.first == _HashTable::OperatorStatus::FAILURE_NEED_EXPAND)
                {
                    beginMoveNode(ReHashStatus::RE_HASHING_EXPAND);
                    return {
                        true,
                        h2.emplace(std::forward <Arg>(arg)...).second
                    };
                }
            }
            return { true, res.second };
        }
        else if (status == ReHashStatus::RE_HASHING_EXPAND)
            return { true, h2.emplace(std::forward <Arg>(arg)...).second };
        else if (status == ReHashStatus::RE_HASHING_SCALE_DOWN)
        {
            auto res = h2.emplace(std::forward <Arg>(arg)...);
            if (res.first == _HashTable::OperatorStatus::FAILURE_KEY_REPEAT)
                return { false, end() };
            else if (res.first == _HashTable::OperatorStatus::FAILURE_NEED_EXPAND)
                PRINT_ERROR("缩容时发生需要扩容事件，重设缩容比例");

            return { true, res.second };
        }

        return { false, end() };
    }

    Iterator find (const _Key &key) noexcept
    {
        moveOneNode();
        if (status == ReHashStatus::DEFAULT)
            return { h1.find(key) };
        else
        {
            auto res = h2.find(key);
            if (res == h2.end())
                return { h1.find(key) };
            else
                return res;
        }
    }

    std::size_t erase (const _Key &key)
    {
        moveOneNode();
        if (status == ReHashStatus::DEFAULT)
        {
            auto res = h1.erase(key);
            if (res.first == _HashTable::OperatorStatus::FAILURE_NEED_SCALE_DOWN)
                beginMoveNode(ReHashStatus::RE_HASHING_SCALE_DOWN);

            return res.second;
        }
        else
        {
            size_t hashCode = h1.getHash(key);
            size_t idx = h1.getHashIndex(hashCode);
            auto p = h1.findBeforeNode(idx, key, hashCode);
            if (p)
            {
                auto res = h2.erase(key);
                if (res.first == _HashTable::OperatorStatus::FAILURE_KEY_NOT_DEFINED)
                    h1.erasePrevNode(idx, p, static_cast<typename _HashTable::NodePtr>(p->next));

                return 1;
            }
            return 0;
        }
    }

    Iterator erase (Iterator it) noexcept
    {
        moveOneNode();
        if (status == ReHashStatus::DEFAULT)
        {
            auto res = h1.erase(it);
            if (res.first == _HashTable::OperatorStatus::FAILURE_NEED_SCALE_DOWN)
                beginMoveNode(ReHashStatus::RE_HASHING_SCALE_DOWN);

            return res.second;
        }
        else
        {
            auto res = h2.erase((*it).first);
            if (res.first == _HashTable::OperatorStatus::FAILURE_KEY_NOT_DEFINED)
                return { h1.erase(it).second };

            return { res.second };
        }
    }

    inline void moveOneNode ()
    {
        if (status == ReHashStatus::DEFAULT)
            return;

        size_t idx = h1.getHashIndex(*rehashMoveNextNode);
        size_t nextIdx;
        decltype(rehashMoveNextNode) oldNext;
        do
        {
            auto newIdx = h2.getHashIndex(*rehashMoveNextNode);
            oldNext = rehashMoveNextNode->getNext();

            if (h2.buckets[newIdx])
            {
                if (static_cast<decltype(rehashMoveNextNode)>(rehashMovePrevNode->next)
                    == rehashMoveNextNode)
                    rehashMovePrevNode->next = rehashMoveNextNode->next;
                rehashMoveNextNode->next = h2.buckets[newIdx]->next;
                h2.buckets[newIdx]->next = rehashMoveNextNode;
            }
            else
            {
                if (static_cast<decltype(rehashMoveNextNode)>(rehashMovePrevNode->next)
                    != rehashMoveNextNode)
                {
                    rehashMoveNextNode->next = rehashMovePrevNode->next;
                    rehashMovePrevNode->next = rehashMoveNextNode;
                }

                h2.buckets[newIdx] = rehashMovePrevNode;
                rehashMovePrevNode = rehashMoveNextNode;
            }

            --h1.elementCount;
            ++h2.elementCount;

            if (!oldNext)
                break;

            rehashMoveNextNode = oldNext;
            nextIdx = h1.getHashIndex(*rehashMoveNextNode);
        } while (idx == nextIdx);

        if (h1.empty())
            endMoveNode();
    }

    inline size_t size () const noexcept
    {
        return h1.size() + h2.size();
    }

    inline size_t empty () const noexcept
    {
        return h1.empty();
    }

    inline Iterator begin () noexcept
    {
        return Iterator(status == ReHashStatus::DEFAULT ? h1.begin() : h2.begin());
    }

    inline Iterator begin () const noexcept
    {
        return ConstIterator(status == ReHashStatus::DEFAULT ? h1.begin() : h2.begin());
    }

    inline Iterator end () noexcept
    {
        return Iterator(static_cast<typename _HashTable::NodePtr>(nullptr));
    }

    inline Iterator end () const noexcept
    {
        return ConstIterator(static_cast<typename _HashTable::NodePtr>(nullptr));
    }

private:
    inline void beginMoveNode (ReHashStatus moveStatus)
    {
        status = moveStatus;

        rehashMovePrevNode = &h1._begin;
        rehashMoveNextNode = static_cast<decltype(rehashMoveNextNode)>(h1._begin.next);
        h2.bucketCount = h1.primeRehashPolicy._M_next_resize;
        h2.buckets = h2.bucketsAllocator.allocateBuckets(h2.bucketCount);

        moveBeginNode();
    }

    inline void moveBeginNode ()
    {
        h2._begin.next = h1._begin.next;

        moveOneNode();
    }

    inline void endMoveNode ()
    {
        status = ReHashStatus::DEFAULT;
        h1.clear();
        h1 = std::move(h2);
    }

private:
    _HashTable h1;
    // rehash table
    _HashTable h2;
    typename _HashTable::NodeBasePtr rehashMovePrevNode;
    typename _HashTable::NodePtr rehashMoveNextNode;
    ReHashStatus status = ReHashStatus::DEFAULT;
};
#endif //LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_
