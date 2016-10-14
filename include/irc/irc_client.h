#pragma once

#include "irc/irc_connection.h"
#include "irc/irc_enums.h"
#include "channel_context.h"

namespace ashbot {

class irc_client : public irc_connection
{
public:
     explicit           irc_client(channel_context& pContext);
public:
    void                login(const char* pNickname, const char* pOAuthToken);
    void                join(const char* pChannel);
    void                send_message(send_type type, const char* pDestination, const char* pMessage);
protected:
    void                ev_line_read(const char* pLine) override;
    void                ev_logged_in() override;
private:  
    void                parse_message(irc_message_data* pData, const char* pLine) const;
    const char*         parse_tags(irc_message_data* pData, const char* pLine) const;
    receive_type        get_message_type(const char* pRawLine, reply_code& pRc) const;
    void                dispatch_message(irc_message_data* pData);

    void                event_ping(irc_message_data* pData);
    void                event_error(irc_message_data* pData);
    void                event_message(irc_message_data* pData);

    static void         unescape_tag_string(std::string& ts);
private:
    channel_context&    channelContext_;
};

}
