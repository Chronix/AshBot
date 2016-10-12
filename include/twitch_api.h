#pragma once

#include "sub_manager.h"

namespace ashbot {
namespace twitch_api {

int get_channel_subscribers(const char* pChannel, int limit, int offset, sub_manager::sub_collection& subs);

}
}