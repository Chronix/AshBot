#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

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

#include "cc_queue.h"
#include "logging.h"
#include "websocketpp.h"
