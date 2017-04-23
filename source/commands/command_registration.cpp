#include "commands/fun_commands.h"
#include "commands/songrequest_commands.h"
#include "channel_context.h"

namespace ashbot {

const command_ptr command_factory::null_command = command_ptr();

void channel_context::register_commands()
{
    using namespace commands;

    commandFactory_.register_command<hello_command>("hello");

    commandFactory_.register_command<songrequest_command>("songrequest", "sr", "requestsong", "srq", "ohgodohgodohgod");
    
    cooldownMap_.reserve(static_cast<size_t>(command_id::max_id));
    bot_clock::time_point lastUse = bot_clock::now() - std::chrono::hours(1);
    
    for (int id = static_cast<int>(command_id::min_id) + 1;
         id < static_cast<int>(command_id::max_id);
         ++id)
    {
        cooldownMap_[static_cast<command_id>(id)] = lastUse;
    }
}

}
