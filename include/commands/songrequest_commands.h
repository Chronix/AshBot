#pragma once

#include "bot_command.h"
#include "command_traits.h"
#include "channel_context.h"
#include "modules/songrequest/songrequest_provider.h"

namespace ashbot {
namespace commands {

constexpr char MSGSTR_SR_DISABLED[] = "Song requests are not enabled right now.";
constexpr char MSGSTR_SR_NO_URL[] = "%s, use !sr <link> to request a song.";
const char* const MSGSTRS_SR_STATE_CHANGES[] = 
{
    "Song request disabled!",
    "Song request enabled!"
};

namespace traits {

using songrequest_command_traits = command_traits<
    command_id::songrequest,
    user_access_level::pleb
>;

}

class songrequest_command : public bot_command_with_args<traits::songrequest_command_traits>
{
public:
    void execute() override
    {
        if (arg_count() == 0)
        {
            if (!context()->songrequest_provider()->enabled()) AshBotSendMessage(MSGSTR_SR_DISABLED);
            else AshBotSendMessage(MSGSTR_SR_NO_URL, user_);
            return;
        }

        if (arg_streq("on")) try_set_sr(true);
        else if (arg_streq("off")) try_set_sr(false);
        else request_song();
        
    }
private:
    void try_set_sr(bool enabled)
    {
        AshBotCommandAccessCheck(user_access_level::broadcaster);
        context()->songrequest_provider()->set_enabled(enabled);
        AshBotSendMessage(MSGSTRS_SR_STATE_CHANGES[enabled]);
    }

    void request_song()
    {
        const char* pLink;
        arg(pLink);
        context()->songrequest_provider()->request_song(user_.get(), pLink);
    }
};
}
}
