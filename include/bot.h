#pragma once

#include "irc/twitch_irc_client.h"
#include "channel_context.h"

namespace ashbot {

class bot
{
public:
                        bot();
public:
    void                run();
    void                stop();
private:
    channel_context     context_;
    twitch_irc_client   client_;
};

}
