#pragma once

#include <boost/asio/io_service.hpp>

namespace ashbot {

namespace detail {

extern boost::asio::io_service Ios;

}

void tp_initialize(int threadCount);
void tp_cleanup();

using tp_work = std::function<void()>;

void tp_queue_work(tp_work&& work);

template<typename _Work>
void tp_queue_work(_Work&& work)
{
    detail::Ios.post(std::move(work));
}

boost::asio::io_service& tp_get_ioservice();

}
