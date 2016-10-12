#pragma once

#include "mutex.h"

namespace ashbot {

template<typename T, size_t _BlockSize = 1, typename TAlloc = std::allocator<T>>
class object_cache
{
    // no fancy crap here
    static_assert(std::is_pod<T>::value, "T must be a POD");
public:
    ~object_cache()
    {
        for (T* pt : blocks_) alloc_.deallocate(pt, sizeof(T));
    }
public:
    T* get_block()
    {
        T* ptr;
        if (freeBlocks_.try_dequeue(ptr)) return ptr;
        ptr = alloc_.allocate(BlockSize * sizeof(T), 0);
        boost::lock_guard<mutex> l(blockMutex_);
        blocks_.push_back(ptr);
        return ptr;
    }

    void release_block(T* block)
    {
        freeBlocks_.enqueue(block);
    }
public:
    static const size_t BlockSize = _BlockSize;
private:
    std::vector<T*> blocks_;
    mutex           blockMutex_;
    cc_queue<T*>    freeBlocks_;
    TAlloc          alloc_;
};

}
