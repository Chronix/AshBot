#pragma once

#include "command_traits.h"
#include "bot_command.h"
#include "channel_context.h"

namespace ashbot {
namespace commands {

namespace traits {
using hello_command_traits = command_traits<
    command_id::roulette,
    user_access_level::dev,
    10
>;
}

class hello_command : public bot_command<traits::hello_command_traits>
{
public:
    void execute() override
    {
        pContext_->send_message(send_type::message, "Hello %s", user_);
    }
};
}
}