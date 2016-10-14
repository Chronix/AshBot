#pragma once

#include <memory>

#include <boost/container/flat_map.hpp>
#include <boost/logic/tribool.hpp>

#include "irc_enums.h"
#include "object_cache.h"
#include "string_pool.h"

namespace ashbot {

class irc_client;
    
struct irc_message_data
{
    using tag_map = boost::container::flat_map<std::string, std::string>;
    using bool3 = boost::logic::tribool;

    // actual max is 25 characters
    enum { MAX_USERNAME_LENGTH = 32 };

    char            username[MAX_USERNAME_LENGTH];
    char            message[ASHBOT_STRING_BUFFER_SIZE];
    receive_type    receiveType;
    reply_code      replyCode;
    tag_map         tags;

    bool3           isSub;
    bool3           isMod;

    bool is_mod()
    {
        if (indeterminate(isMod))
        {
            auto mod = tags.find("mod");
            isMod = mod != tags.end() && mod->second == "1";
        }

        return isMod;
    }

    bool is_sub()
    {
        if (indeterminate(isSub))
        {
            auto sub = tags.find("sub");
            isSub = sub != tags.end() && sub->second == "1";
        }

        return isSub;
    }

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
    isMod = boost::logic::indeterminate;
    isSub = boost::logic::indeterminate;
    
    tags.clear();

    globals::g_messageDataPool->release_block(this);
}

}
