#pragma once

#include "irc/irc_client.h"
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
    irc_client          client_;
};

}
