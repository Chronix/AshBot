#include "channel_context.h"
#include "irc/twitch_irc_client.h"
#include "commands/fun_commands.h"
#include "modules/bot_module.h"
#include "modules/songrequest/songrequest_provider.h"
#include "sub_manager.h"

namespace ashbot {

namespace {

constexpr char DevUsername[] = "n1ghtsh0ck";
constexpr char BroadcasterUsername[] = "ashwinitv";

const boost::posix_time::seconds OneSecond(1);

}

namespace sr = modules::songrequest;

channel_context::channel_context(const char* pChannel)
    :   secondTimer_(tp_get_ioservice(), OneSecond)
    ,   pClient_(nullptr)
    ,   pSongrequest_(new sr::songrequest_provider(this))
    ,   ignorePlebs_(false)
{
    strcpy(channel_, pChannel);
    secondTimer_.async_wait([this](const bs::error_code& ec) {second_timer_elapsed(ec); });
    pSongrequest_->initialize();
}

channel_context::~channel_context()
{
    delete pSongrequest_;
}

void channel_context::stop()
{
    secondTimer_.cancel();
}

void channel_context::process_message(irc_message_data* pMessageData)
{
    if (pMessageData)
    {
        irc_message_data::ptr msg(pMessageData);
        tp_queue_work([this, msg]() { process_message_core(msg); });
    }
}

user_access_level channel_context::get_user_access_level(irc_message_data* pContext) const
{
    if (strcmp(pContext->username(), DevUsername) == 0) return user_access_level::dev;
    if (strcmp(pContext->username(), BroadcasterUsername) == 0) return user_access_level::broadcaster;
    if (pContext->mod()) return user_access_level::moderator;
    if (pContext->sub()) return user_access_level::subscriber;
    if (sub_manager::is_regular(pContext->username(), true)) return user_access_level::regular;
    return user_access_level::pleb;
}

void channel_context::send_message(const char* format, bool action, ...) const
{
    va_list va;
    va_start(va, action);
    
    vsend_message(format, va, action);

    va_end(va);
}

void channel_context::send_message_async(int delayMs, const char* format, bool action, ...) const
{
    va_list va;
    va_start(va, action);
    char* pBuffer = get_string();
    vsnprintf(pBuffer, ASHBOT_STRING_BUFFER_SIZE, format, va);

    auto fn = [=]()
    {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(delayMs));
        pClient_->send_message(action ? send_type::action : send_type::message, channel_, pBuffer);
        release_string(pBuffer);
    };

    execute_async(std::move(fn));
}

void channel_context::vsend_message(const char* format, va_list va, bool action) const
{
    char* pBuffer = get_string();
    vsnprintf(pBuffer, ASHBOT_STRING_BUFFER_SIZE, format, va);
    pClient_->send_message(action ? send_type::action : send_type::message, channel_, pBuffer);
    release_string(pBuffer);
}

void channel_context::timeout_user(const char* pUsername, int seconds, const char* pReason) const
{
    if (pReason) send_message_async(1000, ".timeout %s %i %s", false, pUsername, seconds, pReason);
    else send_message_async(1000, ".timeout %s %i", false, pUsername, seconds);
}

void channel_context::purge_user(const char* pUsername, const char* pReason) const
{
    timeout_user(pUsername, 1, pReason);
}

void channel_context::ban_user(const char* pUsername, const char* pReason) const
{
    if (pReason) send_message_async(1000, ".ban %s %s", false, pUsername, pReason);
    send_message_async(1000, ".ban %s", false, pUsername);
}

void channel_context::second_timer_elapsed(const bs::error_code& ec)
{
    // error checking intentionally omitted, let's hope for the best

    for (modules::timed_module* ptm : timedModules_)
    {
        ptm->timer_second_elapsed();
    }

    secondTimer_.expires_from_now(OneSecond);
    secondTimer_.async_wait([this](const bs::error_code& ec) {second_timer_elapsed(ec); });
}

void channel_context::process_message_core(const irc_message_data::ptr& pMessageData)
{
    command_ptr command = parse_message(pMessageData.get());
    if (command) command->execute();
}

command_ptr channel_context::parse_message(irc_message_data* pMessageData)
{
    if (pMessageData->message()[0] != command_prefix) return command_factory::null_command;

    const char* pKeyword = pMessageData->message() + 1;

    const char* space = strchr(pKeyword, ' ');
    size_t keywordLength = space ? space - pKeyword : strlen(pKeyword);

    user_access_level accessLevel = get_user_access_level(pMessageData);
    if (accessLevel == user_access_level::pleb && ignorePlebs_) return command_factory::null_command;
    if (is_user_ignored(pMessageData->username())) return command_factory::null_command;

    char* _pKeyword = static_cast<char*>(alloca(keywordLength + 1)); // mind the \0
    strncpy(_pKeyword, pKeyword, keywordLength);
    _pKeyword[keywordLength] = 0;

    command_ptr command = commandFactory_.create_command(_pKeyword);
    if (!command) return command; // pretty much same thing as null_command
    if (command->accessible_by(accessLevel))
    {
        command->set_params(this, pMessageData->user());
        command->set_message(pMessageData->message());
        return command;
    }

    // todo: report access denied
    return command_factory::null_command;
}

bool channel_context::is_user_ignored(const char* pUsername)
{
    return false;
}

bool channel_context::is_command_cooldown_ok(command_ptr& command, irc_message_data* pContext)
{
    if (strcmp(pContext->username(), DevUsername) == 0 || strcmp(pContext->username(), BroadcasterUsername) == 0)
    {
        return true;
    }

    boost::upgrade_lock<boost::shared_mutex> sl(cdMutex_);
    
    bot_clock::time_point lastUse = cooldownMap_[command->id()];
    if (bot_clock::now() - lastUse < command->cooldown_)
    {
        return !command->restrictMods_ && pContext->mod();
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> el(sl);
    cooldownMap_[command->id()] = command_clock::now();
    return true;
}
}
