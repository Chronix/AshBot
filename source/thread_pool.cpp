#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "thread_pool.h"

namespace baio = boost::asio;

namespace ashbot {

namespace detail {

baio::io_service        Ios;

}

using detail::Ios;

namespace {

baio::io_service::work  Work(Ios);
boost::thread_group     Threads;

}

void tp_initialize(int threadCount)
{
    for (int i = 0; i < threadCount; ++i)
    {
        Threads.create_thread([]() { Ios.run(); });
    }
}

void tp_cleanup()
{
    Ios.stop();
    Threads.join_all();
}

void tp_queue_work(tp_work&& work)
{
    Ios.post(move(work));
}

boost::asio::io_service& tp_get_ioservice()
{
    return Ios;
}

}
