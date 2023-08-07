//
// Created by Yoshiki on 2023/8/3.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_SKIPLIST_H_
#define LINUX_SERVER_LIB_INCLUDE_SKIPLIST_H_

#include <functional>
#include <memory>
#include "util.h"
template <class T, int MaxLevel = 32, class Compare = std::less <T>>
class SkipList
{
    // static_assert(MaxLevel <= 4 && MaxLevel > 1, "MaxLevel must between 2~4");

    class SkipListNode;
    typedef SkipListNode *SkipListNodePtr;
    typedef SkipList <T, MaxLevel, Compare> ClassType;

    class SkipListNode
    {
    public:
        template <class R>
        SkipListNode (int level, R &&value)
            : value(std::forward <R>(value))
        {
            memset(nextArr, 0, level * sizeof(SkipListNodePtr));
        }
        ~SkipListNode () noexcept = default;
        bool operator== (const T &r) const noexcept
        {
            return value == r;
        }
        bool operator!= (const T &r) const noexcept
        {
            return !operator==(r);
        }
        bool operator== (const SkipListNode &r) const noexcept
        {
            return value == r.value;
        }
        bool operator!= (const SkipListNode &r) const noexcept
        {
            return operator==(r);
        }
        bool operator< (const SkipListNode &r) const noexcept
        {
            return Compare()(value, r.value);
        }
        bool operator< (const T &r) const noexcept
        {
            return Compare()(value, r);
        }
        bool operator> (const SkipListNode &r) const noexcept
        {
            return !operator<(r);
        }
        bool operator> (const T &r) const noexcept
        {
            return !operator<(r);
        }
        void *operator new (size_t size, int level)
        {
            return ::operator new(size + level * sizeof(SkipListNodePtr));
        }

        T value;
        // 1 2 3 4
        SkipListNodePtr nextArr[];
    };

    class SkipListIterator
    {
    public:
        explicit SkipListIterator (const SkipListNodePtr &node)
            : node(node) {}
        ~SkipListIterator () noexcept = default;
        explicit operator bool () const noexcept
        {
            return node != nullptr;
        }
        const T &operator* () const noexcept
        {
            return node->value;
        }
        const T *operator-> () const noexcept
        {
            return Utils::addressOf(node->value);
        }
        bool operator== (const SkipListIterator &r)
        {
            return node == r.node;
        }
        bool operator!= (const SkipListIterator &r)
        {
            return !operator==(r);
        }
        SkipListIterator &operator++ ()
        {
            // 取最低层级的next
            node = node->nextArr[0];
            return *this;
        }
        SkipListIterator operator++ (int)  // NOLINT(cert-dcl21-cpp)
        {
            auto old = *this;
            ++*this;
            return old;
        }

    private:
        SkipListNodePtr node;
    };

public:
    using Iterator = SkipListIterator;
    SkipList () = default;
    ~SkipList () noexcept
    {
        for (SkipListNodePtr i = list[0], old = list[0]; i != nullptr; old = i)
        {
            i = old->nextArr[0];
            delete old;
        }
    };

    Iterator begin () const
    {
        return Iterator(list[0]);
    }

    Iterator end () const noexcept
    {
        return Iterator(nullptr);
    }

    inline size_t size () const noexcept
    {
        return size_;
    }

    template <class R>
    void insert (R &&value)
    {
        int level = getRandomLevel();

        auto *node = new(level) SkipListNode(level, std::forward <R>(value));

        int i = MaxLevel - 1;
        SkipListNodePtr *it = &list[i];

        for (; i >= 0; --i)
        {
            if (*it)
            {
                if (**it > *node)
                {
                    if (i < level)
                    {
                        node->nextArr[i] = *it;
                        *it = node;
                    }
                    it--;
                    continue;
                }

                while ((*it)->nextArr[i] && *((*it)->nextArr[i]) < *node)
                    it = &(*it)->nextArr[i];
                if (i < level)
                {
                    // 如果是**it < *node 退出循环
                    node->nextArr[i] = (*it)->nextArr[i];
                    (*it)->nextArr[i] = node;
                }
                // 将it降层级，it是nextArr[i]，it-- == nextArr[i-1]
                it--;
            }
            else
            {
                if (i < level)
                    *it = node;
                it--;
            }
        }

        if ((*tail)->nextArr[0])
            tail = &(*tail)->nextArr[0];

        size_++;
    }

    bool erase (const T &value)
    {
        int i = MaxLevel - 1;
        SkipListNodePtr *it = &list[i];
        SkipListNodePtr old;

        for (; i >= 0; --i)
        {
            if (*it)
            {
                while ((*it)->nextArr[i] && *(*it)->nextArr[i] < value)
                    it = &(*it)->nextArr[i];

                if ((*it)->nextArr[i] && *(*it)->nextArr[i] == value)
                {
                    if (i != 0)
                        (*it)->nextArr[i] = (*it)->nextArr[i]->nextArr[i];
                    else
                    {
                        if (tail == &(*it)->nextArr[i])
                            tail = it;
                        old = (*it)->nextArr[i];
                        (*it)->nextArr[i] = (*it)->nextArr[i]->nextArr[i];
                        delete old;
                        size_--;
                        return true;
                    }
                }
            }
            it--;
        }

        return false;
    }

    Iterator find (const T &value) const noexcept
    {
        int i = MaxLevel - 1;
        const SkipListNodePtr *ptr = &list[i];
        for (; i >= 0; --i)
        {
            if (*ptr)
            {
                while ((*ptr)->nextArr[i] && **ptr < value)
                    ptr = &(*ptr)->nextArr[i];
                if (**ptr == value)
                    return Iterator(*ptr);
            }

            ptr--;
        }

        return end();
    }

    // void sss () const
    // {
    //     int c = 0;
    //     for (SkipListNodePtr i = list[3]; i != nullptr; i = i->nextArr[3])
    //     {
    //         c++;
    //         std::cout << "level : " << i->level << " value : " << i->value << std::endl;
    //     }
    //     std::cout << c << std::endl;
    // }

private:
    static inline int getRandomLevel () noexcept
    {
        int level = 1;
        while (level <= MaxLevel)
        {
            if (Utils::getRandomNum(0, 3) == 0)
                level++;
            else
                return level;
        }
    }

    // 1 2 3 4
    SkipListNodePtr list[MaxLevel] = { nullptr };
    // 尾部不做层级
    SkipListNodePtr *tail = &list[0];
    size_t size_ = 0;
};
#endif //LINUX_SERVER_LIB_INCLUDE_SKIPLIST_H_
