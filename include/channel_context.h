#pragma once

#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/thread.hpp>

#include "irc/irc_message_data.h"
#include "commands/command_factory.h"
#include "access_level.h"
#include "thread_pool.h"

namespace ashbot {
    
namespace baio = boost::asio;
namespace bs = boost::system;

using namespace commands;

class twitch_irc_client;
enum class send_type;

namespace modules {

class timed_module;

namespace songrequest {

class songrequest_provider;

}

}

class channel_context
{
    using cooldown_map = boost::container::flat_map<command_id, bot_clock::time_point>;
public:
    static constexpr char   command_prefix = '!';
public:
    explicit                channel_context(const char* pChannel);
                            ~channel_context();
public:
    void                    stop();
    void                    set_irc_client(twitch_irc_client* pClient) { pClient_ = pClient; }

    void                    register_commands();

    void                    process_message(irc_message_data* pMessageData);

    user_access_level       get_user_access_level(irc_message_data* pContext) const;

    void                    send_message(const char* format, bool action, ...) const;
    void                    send_message_async(int delayMs, const char* format, bool action, ...) const;
    void                    vsend_message(const char* format, va_list va, bool action = false) const;

    void                    timeout_user(const char* pUsername, int seconds, const char* pReason = nullptr) const;
    void                    purge_user(const char* pUsername, const char* pReason = nullptr) const;
    void                    ban_user(const char* pUsername, const char* pReason = nullptr) const;

    modules::songrequest::songrequest_provider*
                            songrequest_provider() const { return pSongrequest_; }

    template<typename _Function>
    void                    execute_async(_Function&& fn);

    template<typename _Function>
    void                    execute_async(_Function&& fn) const;
private:
    void                    second_timer_elapsed(const bs::error_code& ec);

    void                    process_message_core(const irc_message_data::ptr& pMessageData);
    command_ptr             parse_message(irc_message_data* pMessageData);
    
    bool                    is_user_ignored(const char* pUsername);
    bool                    is_command_cooldown_ok(command_ptr& command, irc_message_data* pContext);
private:
    baio::deadline_timer    secondTimer_;

    twitch_irc_client*      pClient_;
    command_factory         commandFactory_;

    cooldown_map            cooldownMap_;
    boost::shared_mutex     cdMutex_;

    modules::songrequest::songrequest_provider*
                            pSongrequest_;

    std::vector<modules::timed_module*>
                            timedModules_;

    char                    channel_[TWITCH_USERNAME_MAX_LENGTH];

    bool                    ignorePlebs_;
};

template <typename _Function>
void channel_context::execute_async(_Function&& fn)
{
    tp_queue_work(std::forward<_Function>(fn));
}

template <typename _Function>
void channel_context::execute_async(_Function&& fn) const
{
    tp_queue_work(std::forward<_Function>(fn));
}
}
