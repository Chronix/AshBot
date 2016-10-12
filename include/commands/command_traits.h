#pragma once

#include "command_id.h"
#include "access_level.h"

namespace ashbot {
namespace commands {

template<
    command_id _Id,
    user_access_level _AccessLevel,
    size_t _CooldownSeconds,
    bool _RestrictMods = false,
    bool _OfflineChatOnly = false
>
struct command_traits
{
    static const command_id Id = _Id;
    static const user_access_level RequiredAccess = _AccessLevel;
    static const size_t CooldownSeconds = _CooldownSeconds;
    static const bool  RestrictMods = _RestrictMods;
    static const bool OfflineChatOnly = _OfflineChatOnly;
};

}
}
