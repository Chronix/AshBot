#pragma once

#include <vector>

#include "cc_queue.h"
#include "mutex.h"
#include "ref_counted_object.h"

namespace ashbot {

template<typename _T, size_t _BlockSize = 1, typename _TAlloc = std::allocator<_T>>
class object_cache
{
public:
    using alloc_type = _TAlloc;

    ~object_cache()
    {
        for (_T* pt : blocks_)
        {
            alloc_.destroy(pt);
            alloc_.deallocate(pt, sizeof(_T));
        }
    }
public:
    _T* get_block()
    {
        _T* ptr;
        if (freeBlocks_.try_dequeue(ptr)) return ptr;

        ptr = alloc_.allocate(BlockSize * sizeof(_T), 0);
        alloc_.construct(ptr);

        boost::lock_guard<mutex> l(blockMutex_);
        blocks_.push_back(ptr);

        return ptr;
    }

    void release_block(_T* block)
    {
        freeBlocks_.enqueue(block);
    }
public:
    static const size_t BlockSize = _BlockSize;
private:
    std::vector<_T*>    blocks_;
    mutex               blockMutex_;
    cc_queue<_T*>       freeBlocks_;
    _TAlloc             alloc_;
};

template<typename _T>
class ASHBOT_NOVTABLE cached_object : public ref_counted_object
{
protected:
    using cache_type = object_cache<_T>;
protected:
    static _T* get_ptr()
    {
        return Cache.get_block();
    }

    void release_impl(uint32_t newRef) override
    {
        if (newRef == 0)
        {
            Cache.release_block(static_cast<_T*>(this));
        }
    }
private:
    static cache_type   Cache;
};

template<typename _T>
object_cache<_T> cached_object<_T>::Cache;

}
