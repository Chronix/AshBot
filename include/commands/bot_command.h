#pragma once

#include <memory>

#include "access_level.h"
#include "ashbot.h"
#include "command_id.h"
#include "context_object.h"
#include "string_pool.h"
#include "twitch_user.h"

namespace ashbot {
    
class channel_context;

namespace commands {

class bot_command_base : public context_object
{
    friend channel_context;
public:
    virtual                     ~bot_command_base() {}
protected:
                                bot_command_base(command_id id, size_t cooldown,
                                                 user_access_level requiredAccess,
                                                 bool restrictMods, bool offlineOnly);
public:
    virtual void                execute() = 0;
    command_id                  id() const { return id_; }
protected:
    virtual void                set_message(char* pMessage) = 0;
    virtual void                set_params(channel_context* pContext, twitch_user* pUser) = 0;

    bool                        accessible_by(user_access_level level) const;
protected:
    command_id                  id_;
    twitch_user::ptr            user_;
    user_access_level           requiredAccess_;
    bot_clock::duration         cooldown_;
    bool                        restrictMods_;
    bool                        offlineOnly_;
};

#define AshBotCommandAccessCheck(level) do { if (!accessible_by(level)) return; } while (false)

inline bot_command_base::bot_command_base(command_id id, size_t cooldown,
                                          user_access_level requiredAccess,
                                          bool restrictMods, bool offlineOnly)
    :   context_object(nullptr)
    ,   id_(id)
    ,   requiredAccess_(requiredAccess)
    ,   cooldown_(std::chrono::seconds(cooldown))
    ,   restrictMods_(restrictMods)
    ,   offlineOnly_(offlineOnly)
{
}

inline bool bot_command_base::accessible_by(user_access_level level) const
{
    return level >= requiredAccess_;
}

using command_ptr = std::shared_ptr<bot_command_base>;

template<typename _Command>
command_ptr make_command()
{
    return std::make_shared<_Command>();
}

template<typename _Traits>
class bot_command : public bot_command_base
{
protected:
                                bot_command();
protected:
    void                        set_message(char* pMessage) override {}
    void                        set_params(channel_context* pContext, twitch_user* pUser) override;
};

template <typename _Traits>
bot_command<_Traits>::bot_command()
    :   bot_command_base(_Traits::Id, _Traits::CooldownSeconds, _Traits::RequiredAccess,
                         _Traits::RestrictMods, _Traits::OfflineChatOnly)
{
}

template <typename _Traits>
void bot_command<_Traits>::set_params(channel_context* pContext, twitch_user* pUser)
{
    set_context(pContext);
    user_.reset(pUser);
}

template<typename _Traits>
class bot_command_with_args : public bot_command<_Traits>
{
protected:
    size_t                      arg_count() const { return args_.size(); }

    template<typename _TArg>
    std::enable_if_t<std::is_integral<_TArg>::value, bool> arg(_TArg& value, int index = 0)
    {
        if (index >= arg_count()) return false;
        
        const char* argVal = args_[index];
        char* ePtr = nullptr;

        int64_t iValue = strtoll(argVal, &ePtr, 10);

        if (ePtr == argVal) return false;
        value = static_cast<_TArg>(iValue);
        return true;
    }

    bool arg(const char*& value, int index = 0)
    {
        if (index >= arg_count()) return false;
        value = args_[index];
        return true;
    }

    bool arg_streq(const char* pValue, int index = 0)
    {
        return strcmp(args_[index], pValue) == 0;
    }

    void                        set_message(char* pMessage) override;
    void                        set_message_core(char* pMessage);
protected:
    std::vector<const char*>    args_;
};

template <typename _Traits>
void bot_command_with_args<_Traits>::set_message(char* pMessage)
{
    char* pCurrentPos = strchr(pMessage, ' ');
    if (!pCurrentPos) return;
    ++pCurrentPos;

    set_message_core(pCurrentPos);
}

template <typename _Traits>
void bot_command_with_args<_Traits>::set_message_core(char* pMessage)
{
    while (true)
    {
        args_.push_back(pMessage);
        char* pSpace = strchr(pMessage, ' ');
        if (!pSpace) break;
        *pSpace = 0;
        pMessage = pSpace + 1;
    }
}

template<typename _Traits>
class bot_command_with_args_and_message : public bot_command_with_args<_Traits>
{
public:
    virtual                     ~bot_command_with_args_and_message();
protected:
                                bot_command_with_args_and_message();
protected:
    void                        set_message(char* pMessage) override;
protected:
    char*                       pMessage;
};

template <typename _Traits>
bot_command_with_args_and_message<_Traits>::~bot_command_with_args_and_message()
{
    release_string(pMessage);
}

template <typename _Traits>
bot_command_with_args_and_message<_Traits>::bot_command_with_args_and_message()
    :   pMessage(get_string())
{
}

template <typename _Traits>
void bot_command_with_args_and_message<_Traits>::set_message(char* pMessage)
{
    char* pActualMessage = strchr(pMessage, ' ');
    if (!pActualMessage) return;
    ++pActualMessage;

    strcpy(pMessage, pActualMessage);
    this->set_message_core(pActualMessage); // clang doesn't like this without 'this->'
}
}
}
