#pragma once

#include "irc_client.h"

namespace ashbot {

class twitch_irc_client : public irc_client
{
public:
    enum class notice_id
    {
        submode_on,
        submode_already_on,
        submode_off,
        submode_already_off,
        slowmode_on,
        slowmode_off,
        r9k_on,
        r9k_already_on,
        r9k_off,
        r9k_already_off,
        host_on,
        host_already_on,
        host_off,
        hosts_remaining,
        emote_only_on,
        emote_only_already_on,
        emote_only_off,
        emote_only_already_off,
        channel_suspended,
        timeout_success,
        user_ban_success,
        user_unban_success,
        user_not_banned,
        user_already_banned,
        unrecognized_command
    };
public:
    explicit        twitch_irc_client(channel_context& context);
protected:
    receive_type    get_message_type(const char* pRawLine, reply_code& rc) const override;
    void            dispatch_message(irc_message_data* pData) override;
private:
    void            event_notice(irc_message_data* pData);
    void            event_hosttarget(irc_message_data* pData);
    void            event_clearchat(irc_message_data* pData);
    void            event_userstate(irc_message_data* pData);
    void            event_reconnect(irc_message_data* pData);
    void            event_roomstate(irc_message_data* pData);
    void            event_usernotice(irc_message_data* pData);
};

}
