#pragma once

#include <concurrentqueue/concurrentqueue.h>
#include <concurrentqueue/blockingconcurrentqueue.h>

namespace ashbot {

template<typename T>
using cc_queue = moodycamel::ConcurrentQueue<T>;

template<typename T>
using blocking_queue = moodycamel::BlockingConcurrentQueue<T>;

}
