#pragma once

#include <memory>

#define ASHBOT_STRING_BUFFER_SIZE      2048

#include "object_cache.h"

namespace ashbot {

using string_pool = object_cache<char, ASHBOT_STRING_BUFFER_SIZE>;

namespace globals {
extern std::unique_ptr<string_pool> g_stringPool;
}

inline char* get_string()
{
    return globals::g_stringPool->get_block();
}

inline void release_string(char* str)
{
    if (str) globals::g_stringPool->release_block(str);
}

}
