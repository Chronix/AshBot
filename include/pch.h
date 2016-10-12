#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

#include <boost/asio.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/date_time.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/thread.hpp>
#include <boost/xpressive/xpressive.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "logging.h"
#include "concurrent_queue.h"
#include "blocking_queue.h"

template<typename T>
using cc_queue = moodycamel::ConcurrentQueue<T>;

template<typename T>
using blocking_queue = moodycamel::BlockingConcurrentQueue<T>;
