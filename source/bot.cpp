#include "bot.h"
#include "irc/irc_debug_data.h"

namespace ashbot {

bot::bot()
    :   context_(IRC_CHANNEL)
    ,   client_(context_)
{
    context_.set_irc_client(&client_);
    context_.register_commands();
}

void bot::run()
{
    client_.connect();
    client_.login(IRC_NICKNAME, IRC_TOKEN);
    client_.join(IRC_CHANNEL);
}

void bot::stop()
{
    context_.stop();
    client_.stop();
}
}
