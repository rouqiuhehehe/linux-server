//
// Created by Administrator on 2023/4/23.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_BLOOM_FILTER_H_
#define LINUX_SERVER_LIB_INCLUDE_BLOOM_FILTER_H_

#include <limits>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>

#include <iostream>

static const std::size_t bitsPerChar = 0x08;    // 8 bits in 1 char(unsigned)

static const unsigned char bitMask[bitsPerChar] = {
    0x01,  //00000001
    0x02,  //00000010
    0x04,  //00000100
    0x08,  //00001000
    0x10,  //00010000
    0x20,  //00100000
    0x40,  //01000000
    0x80   //10000000
};

typedef unsigned long long NumType;
template <NumType num, NumType falsePositiveRate = num, NumType seed = 0xA5A5A5A55A5A5A5AULL>
class BloomFilter
{
    static_assert(falsePositiveRate > 1, "false positive rate must > 1");
protected:
    typedef unsigned BloomSaltType;
    typedef unsigned char TableType;
public:
    BloomFilter ();
    BloomFilter (const BloomFilter &filter);
    virtual ~BloomFilter () = default;

    inline bool operator! ();
    inline bool operator== (const BloomFilter &filter);
    inline bool operator!= (const BloomFilter &filter);
    inline BloomFilter &operator= (const BloomFilter &filter);

public:
    inline NumType size () const { return tableSize; };
    inline NumType elementCount () const { return insertElementCount; };
    inline double effectiveFalsePositiveRate () const
    {
        // 计算实时假阳率
        return std::pow(
            1.0 - std::exp(-1.0 * salt.size() * insertElementCount / size()), 1.0 * salt.size()
        );
    };
    inline const TableType *tableData () const { return bitTable.data(); };
    inline std::size_t hashCount () const { return numOfHash; };

    inline void clear ();

    inline void insert (const TableType *key, std::size_t len);
    template <class T>
    inline void insert (const T &t) { insert(reinterpret_cast<const TableType *>(&t), sizeof(T)); }
    inline void insert (const std::string &t) { insert(t.c_str(), t.size()); };
    inline void insert (
        const char *data,
        std::size_t len
    ) { insert(reinterpret_cast<const unsigned char *>(data), len); }
    template <class InputIterator>
    inline void insert (const InputIterator &begin, const InputIterator &end)
    {
        InputIterator it = begin;

        while (it != end)
            insert(*(it++));
    }

    inline virtual bool contains (const TableType *key, std::size_t len) const;
    template <class T>
    inline bool contains (const T &t) const
    {
        return contains(
            reinterpret_cast<const TableType *>(t),
            sizeof(T));
    };
    inline bool contains (const std::string &t) const { return contains(t.data(), t.size()); };
    inline bool contains (
        const char *data,
        std::size_t len
    ) const { return contains(reinterpret_cast<const unsigned char *>(data), len); };

    template <class InputIterator>
    inline bool containsAll (const InputIterator &begin, const InputIterator &end) const
    {
        InputIterator it = begin;
        while (it != end)
        {
            if (!contains(*(it++)))
                return false;
        }

        return true;
    }

    template <class InputIterator>
    inline bool containsNone (const InputIterator &begin, const InputIterator &end) const
    {
        InputIterator it = begin;
        while (it != end)
        {
            if (contains(*(it++)))
                return false;
        }

        return true;
    }

    template <class InputIterator>
    inline bool containsSome (const InputIterator &begin, const InputIterator &end) const
    {
        InputIterator it = begin;
        while (it != end)
        {
            if (contains(*(it++)))
                return true;
        }

        return false;
    }

protected:
    virtual void computeOptimalParameters ();
    // 生成随机盐，即随机hash函数
    virtual inline void generateUniqueSalt ();

    // hash函数
    virtual inline BloomSaltType hash (
        const unsigned char *begin,
        std::size_t remaining_length,
        BloomSaltType hash
    ) const;

    virtual inline void computeIndices (
        const BloomSaltType &hash,
        std::size_t &tableIndex,
        std::size_t &bitIndex
    ) const
    {
        tableIndex = hash % tableSize;
        bitIndex = tableIndex % bitsPerChar;
    }

private:
    NumType randomSeed = seed * 0xA5A5A5A5 + 1;
    // 假阳率
    double falsePositiveProbability = (double)1 / falsePositiveRate;
    // 预计插入元素个数
    NumType projectedElementCount = num;
    // hash函数数量
    unsigned numOfHash = 0;
    // 位图大小
    NumType tableSize = 0;
    // 实际插入元素个数
    NumType insertElementCount = 0;
    std::vector <BloomSaltType> salt;
    std::vector <TableType> bitTable;
};

template <NumType num, NumType falsePositiveRate, NumType seed>
BloomFilter <num, falsePositiveRate, seed>::BloomFilter ()
{
    computeOptimalParameters();
    generateUniqueSalt();

    bitTable.resize(tableSize / bitsPerChar, 0);
}
template <NumType num, NumType falsePositiveRate, NumType seed>
BloomFilter <num, falsePositiveRate, seed>::BloomFilter (const BloomFilter &filter)
{
    this->operator=(filter);
}
template <NumType num, NumType falsePositiveRate, NumType seed>
bool BloomFilter <num, falsePositiveRate, seed>::operator! ()
{

    return (projectedElementCount == 0) ||
        (falsePositiveProbability < 0.0) ||
        (std::numeric_limits <double>::infinity()
            == std::abs(falsePositiveProbability)) ||
        (randomSeed == 0) ||
        (randomSeed == 0xFFFFFFFFFFFFFFFFULL);
}
template <NumType num, NumType falsePositiveRate, NumType seed>
void BloomFilter <num, falsePositiveRate, seed>::computeOptimalParameters ()
{

    if (!(*this))
        return;

    double min_m = std::numeric_limits <double>::infinity();
    double min_k = 0.0;
    double k = 1.0;

    while (k < 1000.0)
    {
        const double numerator = (-k * projectedElementCount);
        const double denominator =
            std::log(1.0 - std::pow(falsePositiveProbability, 1.0 / k));

        const double curr_m = numerator / denominator;

        if (curr_m < min_m)
        {
            min_m = curr_m;
            min_k = k;
        }

        k += 1.0;
    }

    numOfHash = static_cast<unsigned int>(min_k);
    tableSize = static_cast<unsigned long long>(min_m);

    // 做填充，保证被8除尽
    tableSize += (tableSize % bitsPerChar == 0) ? 0 : (bitsPerChar - tableSize % bitsPerChar);
}
template <NumType num, NumType falsePositiveRate, NumType seed>
void BloomFilter <num, falsePositiveRate, seed>::generateUniqueSalt ()
{
    const unsigned int predefinedSaltCount = 128;

    static const BloomSaltType predefinedSalt[predefinedSaltCount] = {
        0xAAAAAAAA, 0x55555555, 0x33333333, 0xCCCCCCCC,
        0x66666666, 0x99999999, 0xB5B5B5B5, 0x4B4B4B4B,
        0xAA55AA55, 0x55335533, 0x33CC33CC, 0xCC66CC66,
        0x66996699, 0x99B599B5, 0xB54BB54B, 0x4BAA4BAA,
        0xAA33AA33, 0x55CC55CC, 0x33663366, 0xCC99CC99,
        0x66B566B5, 0x994B994B, 0xB5AAB5AA, 0xAAAAAA33,
        0x555555CC, 0x33333366, 0xCCCCCC99, 0x666666B5,
        0x9999994B, 0xB5B5B5AA, 0xFFFFFFFF, 0xFFFF0000,
        0xB823D5EB, 0xC1191CDF, 0xF623AEB3, 0xDB58499F,
        0xC8D42E70, 0xB173F616, 0xA91A5967, 0xDA427D63,
        0xB1E8A2EA, 0xF6C0D155, 0x4909FEA3, 0xA68CC6A7,
        0xC395E782, 0xA26057EB, 0x0CD5DA28, 0x467C5492,
        0xF15E6982, 0x61C6FAD3, 0x9615E352, 0x6E9E355A,
        0x689B563E, 0x0C9831A8, 0x6753C18B, 0xA622689B,
        0x8CA63C47, 0x42CC2884, 0x8E89919B, 0x6EDBD7D3,
        0x15B6796C, 0x1D6FDFE4, 0x63FF9092, 0xE7401432,
        0xEFFE9412, 0xAEAEDF79, 0x9F245A31, 0x83C136FC,
        0xC3DA4A8C, 0xA5112C8C, 0x5271F491, 0x9A948DAB,
        0xCEE59A8D, 0xB5F525AB, 0x59D13217, 0x24E7C331,
        0x697C2103, 0x84B0A460, 0x86156DA9, 0xAEF2AC68,
        0x23243DA5, 0x3F649643, 0x5FA495A8, 0x67710DF8,
        0x9A6C499E, 0xDCFB0227, 0x46A43433, 0x1832B07A,
        0xC46AFF3C, 0xB9C8FFF0, 0xC9500467, 0x34431BDF,
        0xB652432B, 0xE367F12B, 0x427F4C1B, 0x224C006E,
        0x2E7E5A89, 0x96F99AA5, 0x0BEB452A, 0x2FD87C39,
        0x74B2E1FB, 0x222EFD24, 0xF357F60C, 0x440FCB1E,
        0x8BBE030F, 0x6704DC29, 0x1144D12F, 0x948B1355,
        0x6D8FD7E9, 0x1C11A014, 0xADD1592F, 0xFB3C712E,
        0xFC77642F, 0xF9C4CE8C, 0x31312FB9, 0x08B0DD79,
        0x318FA6E7, 0xC040D23D, 0xC0589AA7, 0x0CA5C075,
        0xF874B172, 0x0CF914D5, 0x784D3280, 0x4E8CFEBC,
        0xC569F575, 0xCDB2A091, 0x2CC016B4, 0x5C5F4421
    };

    if (numOfHash < predefinedSaltCount)
    {
        std::copy(predefinedSalt, predefinedSalt + numOfHash, std::back_inserter(salt));

        for (std::size_t i = 0; i < salt.size(); ++i)
            salt[i] =
                salt[i] * salt[(i + 3) % salt.size()] + static_cast<BloomSaltType>(randomSeed);
    }
        // 如果提供的盐小于需要的hash函数数量，则随即填充
    else
    {
        std::copy(predefinedSalt, predefinedSalt + predefinedSaltCount, std::back_inserter(salt));

        srand(randomSeed);
        while (salt.size() < numOfHash)
        {
            BloomSaltType currSalt = rand() * rand();
            if (currSalt == 0)
                continue;

            if (std::find(salt.begin(), salt.end(), currSalt) == salt.end())
                salt.push_back(currSalt);
        }
    }
}
template <NumType num, NumType falsePositiveRate, NumType seed>
bool BloomFilter <num, falsePositiveRate, seed>::operator== (const BloomFilter &filter)
{
    if (&filter == this)
        return true;

    return (salt == filter.salt)
        && (randomSeed == filter.randomSeed)
        && (falsePositiveProbability == filter.falsePositiveProbability)
        && (projectedElementCount == filter.projectedElementCount)
        && (numOfHash == filter.numOfHash)
        && (tableSize == filter.tableSize)
        && (insertElementCount == filter.insertElementCount)
        && (bitTable == filter.bitTable);
}
template <NumType num, NumType falsePositiveRate, NumType seed>
bool BloomFilter <num, falsePositiveRate, seed>::operator!= (const BloomFilter &filter)
{
    return !(*this == filter);
}
template <NumType num, NumType falsePositiveRate, NumType seed>
BloomFilter <num, falsePositiveRate, seed> &
BloomFilter <num, falsePositiveRate, seed>::operator= (const BloomFilter &filter)
{
    if (this != &filter)
    {
        salt = filter.salt;
        tableSize = filter.tableSize;
        numOfHash = filter.numOfHash;
        bitTable = filter.bitTable;

        projectedElementCount = filter.projectedElementCount;
        insertElementCount = filter.insertElementCount;
        randomSeed = filter.randomSeed;
        falsePositiveProbability = filter.falsePositiveProbability;
    }

    return *this;
}
template <NumType num, NumType falsePositiveRate, NumType seed>
void BloomFilter <num, falsePositiveRate, seed>::clear ()
{
    std::fill(bitTable.begin(), bitTable.end(), 0);
    insertElementCount = 0;
}
template <NumType num, NumType falsePositiveRate, NumType seed>
typename BloomFilter <num, falsePositiveRate, seed>::BloomSaltType BloomFilter <num,
                                                                                falsePositiveRate,
                                                                                seed>::hash (
    const unsigned char *begin,
    std::size_t remaining_length,
    BloomFilter::BloomSaltType hash
) const
{
    const unsigned char *itr = begin;
    unsigned int loop = 0;

    while (remaining_length >= 8)
    {
        const unsigned int &i1 = *(reinterpret_cast<const unsigned int *>(itr));
        itr += sizeof(unsigned int);
        const unsigned int &i2 = *(reinterpret_cast<const unsigned int *>(itr));
        itr += sizeof(unsigned int);

        hash ^= (hash << 7) ^ i1 * (hash >> 3) ^
            (~((hash << 11) + (i2 ^ (hash >> 5))));

        remaining_length -= 8;
    }

    if (remaining_length)
    {
        if (remaining_length >= 4)
        {
            const unsigned int &i = *(reinterpret_cast<const unsigned int *>(itr));

            if (loop & 0x01)
                hash ^= (hash << 7) ^ i * (hash >> 3);
            else
                hash ^= (~((hash << 11) + (i ^ (hash >> 5))));

            ++loop;

            remaining_length -= 4;

            itr += sizeof(unsigned int);
        }

        if (remaining_length >= 2)
        {
            const unsigned short &i = *(reinterpret_cast<const unsigned short *>(itr));

            if (loop & 0x01)
                hash ^= (hash << 7) ^ i * (hash >> 3);
            else
                hash ^= (~((hash << 11) + (i ^ (hash >> 5))));

            ++loop;

            remaining_length -= 2;

            itr += sizeof(unsigned short);
        }

        if (remaining_length)
        {
            hash += ((*itr) ^ (hash * 0xA5A5A5A5)) + loop;
        }
    }

    return hash;
}
template <NumType num, NumType falsePositiveRate, NumType seed>
void BloomFilter <num, falsePositiveRate, seed>::insert (
    const BloomFilter::TableType *key,
    std::size_t len
)
{
    std::size_t tableIndex = 0;
    std::size_t bitIndex = 0;
    BloomSaltType hashcode;

    for (std::size_t i = 0; i < numOfHash; ++i)
    {
        hashcode = hash(key, len, salt[i]);
        computeIndices(hashcode, tableIndex, bitIndex);

        bitTable[tableIndex / bitsPerChar] |= bitMask[bitIndex];
    }

    ++insertElementCount;
}
template <NumType num, NumType falsePositiveRate, NumType seed>
bool BloomFilter <num, falsePositiveRate, seed>::contains (
    const BloomFilter::TableType *key,
    const std::size_t len
) const
{
    std::size_t tableIndex = 0;
    std::size_t bitIndex = 0;
    BloomSaltType hashcode;

    for (int i = 0; i < numOfHash; ++i)
    {
        hashcode = hash(key, len, salt[i]);
        computeIndices(hashcode, tableIndex, bitIndex);

        if ((bitTable[tableIndex / bitsPerChar] & bitMask[bitIndex]) != bitMask[bitIndex])
            return false;
    }

    return true;
}

#endif //LINUX_SERVER_LIB_INCLUDE_BLOOM_FILTER_H_
