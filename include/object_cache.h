#pragma once

#include <vector>

#include "cc_queue.h"
#include "mutex.h"

namespace ashbot {

template<typename T, size_t _BlockSize = 1, typename TAlloc = std::allocator<T>>
class object_cache
{
    static_assert(std::is_default_constructible<T>::value, "T must be default-constructible");
public:
    ~object_cache()
    {
        for (T* pt : blocks_)
        {
            alloc_.destroy(pt);
            alloc_.deallocate(pt, sizeof(T));
        }
    }
public:
    T* get_block()
    {
        T* ptr;
        if (freeBlocks_.try_dequeue(ptr)) return ptr;

        ptr = alloc_.allocate(BlockSize * sizeof(T), 0);
        alloc_.construct(ptr);

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
