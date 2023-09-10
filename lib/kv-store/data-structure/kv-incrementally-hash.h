//
// Created by 115282 on 2023/8/28.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_

#include "kv-hash-table.h"

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
    public:
        _IncrementallyHashTableIterator (
            IncrementallyHashTable *table,
            _HashTable *h,
            const typename _HashTable::Iterator &it
        )
            : table(table), h(h), _HashTable::Iterator(it) {};

        _IncrementallyHashTableIterator ()
            : _HashTable::Iterator(nullptr) {};

        inline _IncrementallyHashTableIterator &operator++ () noexcept override
        {
            auto it = _HashTable::Iterator::operator++();
            if (it.operator->() == nullptr && !table->h2.empty() && h == table->h1)
            {
                this->node = table->h2.begin().node;
                h = &table->h2;
            }

            return *this;
        }

        inline _IncrementallyHashTableIterator &operator++ (int) noexcept override // NOLINT
        {
            auto old = *this;
            ++(*this);

            return old;
        }

    private:
        _HashTable *h;
        IncrementallyHashTable *table;
    };

public:
    using Iterator = _IncrementallyHashTableIterator;
    using ConstIterator = const _IncrementallyHashTableIterator;

    IncrementallyHashTable () = default;
    ~IncrementallyHashTable () = default;

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
        return Iterator(this, &h1, h1.begin());
    }

    inline Iterator begin () const noexcept
    {
        return ConstIterator(this, &h1, h1.begin());
    }

    inline Iterator end () noexcept
    {
        return Iterator(this, &h2, h2.end());
    }

    inline Iterator end () const noexcept
    {
        return ConstIterator(this, &h2, h2.end());
    }

private:
    _HashTable h1;
    // rehash table
    _HashTable h2;
    ReHashStatus status = ReHashStatus::DEFAULT;
};
#endif //LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_
