#pragma once

#include <memory>

#include "irc_enums.h"
#include "object_cache.h"
#include "string_pool.h"

namespace ashbot {

class irc_client;
    
struct irc_message_data
{
    // actual max is 25 characters
    enum { MAX_USERNAME_LENGTH = 32 };

    char            username[MAX_USERNAME_LENGTH];
    char            message[ASHBOT_STRING_BUFFER_SIZE];
    bool            isMod;
    bool            isSub;
    receive_type    receiveType;
    reply_code      replyCode;

    static irc_message_data* get();
    void            release();
};

using message_data_pool = object_cache<irc_message_data>;

namespace globals {
extern std::unique_ptr<message_data_pool> g_messageDataPool;
}

inline irc_message_data* irc_message_data::get()
{
    irc_message_data* pData = globals::g_messageDataPool->get_block();
    return pData;
}

inline void irc_message_data::release()
{
    globals::g_messageDataPool->release_block(this);
}

}
