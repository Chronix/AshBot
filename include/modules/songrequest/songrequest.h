#pragma once

#include <chrono>

namespace ashbot {
namespace modules {
namespace songrequest {

using clock_type = std::chrono::system_clock;
using song_duration = clock_type::duration;

}
}
}
