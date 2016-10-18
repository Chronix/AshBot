#pragma once

// im really triggered by the fact that neither
// boost::(recursive)_mutex nor std::mutex are
// implemented using CRITICAL_SECTION on Windows

// hence this

#include <boost/noncopyable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace ashbot {
    
#ifdef WIN32
class mutex : boost::noncopyable
{
public:
    using native_handle_type = CRITICAL_SECTION*;
    using scoped_lock = boost::unique_lock<mutex>;
public:
    mutex()
    {
        InitializeCriticalSection(&cs_);
    }

    ~mutex()
    {
        DeleteCriticalSection(&cs_);
    }
public:
    void lock()
    {
        EnterCriticalSection(&cs_);
    }

    bool try_lock() noexcept
    {
        return TryEnterCriticalSection(&cs_) != FALSE;
    }

    void unlock()
    {
        LeaveCriticalSection(&cs_);
    }

    native_handle_type native_handle()
    {
        return &cs_;
    }
private:
    CRITICAL_SECTION    cs_;
};
#else
using mutex = boost::recursive_mutex;
#endif

}
