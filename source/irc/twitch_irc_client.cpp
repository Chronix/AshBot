#include "irc/twitch_irc_client.h"
#include "util.h"

namespace ashbot {

twitch_irc_client::twitch_irc_client(channel_context& context)
    :   irc_client(context)
{
}

receive_type twitch_irc_client::get_message_type(const char* pRawLine, reply_code& rc) const
{
    rc = reply_code::null;

    if (strstr(pRawLine, "HOSTTARGET")) return receive_type::hosttarget;
    if (strstr(pRawLine, "CLEARCHAT")) return receive_type::clearchat;
    if (strstr(pRawLine, "USERSTATE")) return receive_type::userstate;
    if (strstr(pRawLine, "RECONNECT")) return receive_type::reconnect;
    if (strstr(pRawLine, "ROOMSTATE")) return receive_type::roomstate;
    if (strstr(pRawLine, "USERNOTICE")) return receive_type::usernotice;

    return irc_client::get_message_type(pRawLine, rc);
}

void twitch_irc_client::dispatch_message(irc_message_data* pData)
{
    using event_fn = void(twitch_irc_client::*)(irc_message_data*);
    static constexpr event_fn handlers[] =
    {
        &twitch_irc_client::event_notice,
        &twitch_irc_client::event_hosttarget,
        &twitch_irc_client::event_clearchat,
        &twitch_irc_client::event_userstate,
        &twitch_irc_client::event_reconnect,
        &twitch_irc_client::event_roomstate,
        &twitch_irc_client::event_usernotice
    };

    static constexpr int start = static_cast<int>(receive_type::channel_notice);
    static constexpr int end = static_cast<int>(receive_type::usernotice);

    int rt = static_cast<int>(pData->receiveType);
    assert(static_cast<unsigned>(rt - start) < array_size(handlers));
    if (rt >= start && rt <= end) std::invoke(handlers[rt - start], this, pData);
    else irc_client::dispatch_message(pData);
}

// to be filled as needed later

void twitch_irc_client::event_notice(irc_message_data* pData)
{
}

void twitch_irc_client::event_hosttarget(irc_message_data* pData)
{
}

void twitch_irc_client::event_clearchat(irc_message_data* pData)
{
}

void twitch_irc_client::event_userstate(irc_message_data* pData)
{
}

void twitch_irc_client::event_reconnect(irc_message_data* pData)
{
}

void twitch_irc_client::event_roomstate(irc_message_data* pData)
{
}

void twitch_irc_client::event_usernotice(irc_message_data* pData)
{
}
}
