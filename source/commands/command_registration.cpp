#include "commands/fun_commands.h"
#include "channel_context.h"

namespace ashbot {

const command_ptr command_factory::null_command = command_ptr();

void channel_context::register_commands()
{
    using namespace commands;

    commandFactory_.register_command<hello_command>("hello");

    cooldownMap_.reserve(static_cast<size_t>(command_id::max_id));
    command_clock::time_point lastUse = command_clock::now() - std::chrono::hours(1);
    
    for (int id = static_cast<int>(command_id::min_id) + 1;
         id < static_cast<int>(command_id::max_id);
         ++id)
    {
        cooldownMap_[static_cast<command_id>(id)] = lastUse;
    }
}

}
