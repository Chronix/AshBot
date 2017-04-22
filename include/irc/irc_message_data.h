#pragma once

#include <memory>

#include <boost/container/flat_map.hpp>

#include "irc_enums.h"
#include "object_cache.h"
#include "string_pool.h"
#include "twitch_user.h"

namespace ashbot {

class irc_message_data final : public cached_object<irc_message_data>
{
    friend cache_type::alloc_type;
    friend class irc_client;
    friend class twitch_irc_client;

    using base_type = cached_object<irc_message_data>;
    using tag_map = boost::container::flat_map<std::string, std::string>;
public:
    using ptr = boost::intrusive_ptr<irc_message_data>;
private:
    irc_message_data() = default;
    virtual ~irc_message_data() {}
public:
    static ptr create()
    {
        return ptr(get_ptr());
    }

    twitch_user* user() const { return user_.get(); }
    const char* username() const { return user_->username(); }
    bool mod() const { return user_->mod(); }
    bool sub() const { return user_->sub(); }
    char* message() { return message_; }
    receive_type get_receive_type() const { return receiveType_; }
protected:
    void release_impl(uint32_t newRef) override
    {
        if (newRef == 0)
        {
            user_.reset();
            tags_.clear();
        }

        base_type::release_impl(newRef);
    }
private:
    bool requires_user_id() const
    {
        // to be modified as needed
        switch (receiveType_)
        {
        case receive_type::channel_message:
        case receive_type::channel_action:
            return true;
        default: return false;
        }
    }

    void update_user(const char* pUsername, size_t usernameLen)
    {
        user_ = twitch_user::create();

        auto it = tags_.find("mod");
        bool mod = it != tags_.end() && it->second == "1";

        it = tags_.find("sub");
        bool sub = it != tags_.end() && it->second == "1";

        it = tags_.find("display-name");
        std::string displayName;
        if (it != tags_.end()) displayName.swap(it->second);
        // we can just take it             ^^^^^^^^^^^^^^^

        it = tags_.find("user-id");
        if (requires_user_id() && it == tags_.end())
        {
            AshBotLogFatal << "Message does not contain user-id";
            abort();
        }

        user_->set(it != tags_.end() ? stoll(it->second) : 0,
                   pUsername, usernameLen, move(displayName), mod, sub);
    }
private:
    // this is a little derp in the design because we have
    // "irc_client" and "twitch_irc_client" but not "irc_message_data"
    // (with just "class user") and "twitch_irc_message_data"
    // this means that irc_message_data is coupled with twitch
    // even though it shouldn't
    // maybe I should just remove irc_client

    // twitch_user has to be ref-counted because commands/modules
    // can hold it beyond the lifetime of its irc_message_data
    twitch_user::ptr    user_;
    char                message_[ASHBOT_STRING_BUFFER_SIZE];
    receive_type        receiveType_;
    reply_code          replyCode_;
    tag_map             tags_;
};

}
