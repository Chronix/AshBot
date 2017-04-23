#pragma once

#include <stdint.h>

#include <chrono>

#define ASHBOT_CHANNEL      "ashwinitv"
#define STREAMER_COUNTRY    "CZ"
#define SR_BASE_URL         "http://192.168.1.115:8085/SongRequest/"

#ifdef _MSC_VER
# define ASHBOT_NOVTABLE __declspec(novtable)
#else
# define ASHBOT_NOVTABLE
#endif

namespace ashbot {

using user_id = int64_t;
using bot_clock = std::chrono::steady_clock;

}
