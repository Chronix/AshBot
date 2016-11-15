#pragma once

#include <string>

#include "object_cache.h"

#define TWITCH_USERNAME_MAX_LENGTH 32 // it's actually 25

namespace ashbot {

class twitch_user final : public cached_object<twitch_user>
{
    friend cache_type::alloc_type;
    friend class irc_message_data;
public:
    using ptr = boost::intrusive_ptr<twitch_user>;
private:
    twitch_user() = default;
    ~twitch_user() {}
public:
    static ptr create()
    {
        return ptr(get_ptr());
    }

    const char* username() const { return username_; }
    bool mod() const { return moderator_; }
    bool sub() const { return subscriber_; }
private:
    void set(const char* pUsername, size_t usernameLen, std::string&& pDisplayName, bool mod, bool sub)
    {
        assert(usernameLen < TWITCH_USERNAME_MAX_LENGTH);
        memcpy(username_, pUsername, usernameLen);
        username_[usernameLen] = 0;
        displayName_.assign(move(pDisplayName));
        moderator_ = mod;
        subscriber_ = sub;
    }
private:
    char                        username_[TWITCH_USERNAME_MAX_LENGTH];
    // this will be a dupe for most users, but it will be swapped from
    // irc_message_data::tags_["display-name"] so no actual memory is wasted
    std::string                 displayName_;
    bool                        moderator_;
    bool                        subscriber_;
    // don't store regular status since it's not a part of the irc message and there's
    // no need to go to the db if we don't need the status most of the time anyway
};

}
