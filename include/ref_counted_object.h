#pragma once

#include <stdint.h>

#include <atomic>

#include "ashbot.h"

namespace ashbot {

class ASHBOT_NOVTABLE ref_counted_object
{
    template<typename T>
    friend class boost::intrusive_ptr;
protected:
    ref_counted_object() : refs_(0) {}
    virtual ~ref_counted_object() {}
public:
    uint32_t add_ref()
    {
        uint32_t newRef = ++refs_;
        add_ref_impl(newRef);
        return newRef;
    }

    uint32_t release()
    {
        uint32_t newRef = --refs_;
        release_impl(newRef);
        return newRef;
    }
protected:
    virtual void add_ref_impl(uint32_t newRef) {}
    virtual void release_impl(uint32_t newRef) {}

    friend void intrusive_ptr_add_ref(ref_counted_object* po)
    {
        po->add_ref();
    }

    friend void intrusive_ptr_release(ref_counted_object* po)
    {
        po->release();
    }
private:
    std::atomic<uint32_t>   refs_;
};

}
