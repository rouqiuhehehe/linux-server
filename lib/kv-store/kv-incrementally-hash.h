//
// Created by 115282 on 2023/8/28.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_
#define LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_

#include <unordered_map>
template <typename _Key, typename _Tp,
    typename _Hash = std::hash <_Key>,
    typename _Pred = std::equal_to <_Key>,
    typename _Alloc = std::allocator <std::pair <const _Key, _Tp>>>
class IncrementallyHash
{
    using HashTable = std::__umap_hashtable <_Key, _Tp, _Hash, _Pred, _Alloc>;

private:
    HashTable hashTable;
    HashTable newHashTable;

};
#endif //LINUX_SERVER_LIB_KV_STORE_KV_INCREMENTALLY_HASH_H_
