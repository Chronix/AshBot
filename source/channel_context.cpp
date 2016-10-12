#include "channel_context.h"
#include "irc/irc_client.h"
#include "commands/fun_commands.h"
#include "modules/bot_module.h"

namespace ashbot {

namespace {

constexpr char DevUsername[] = "n1ghtsh0ck";
constexpr char BroadcasterUsername[] = "ashwinitv";

const boost::posix_time::seconds OneSecond(1);

}

channel_context::channel_context(const char* pChannel, int threadCount)
    :   secondTimer_(tp_get_ioservice(), OneSecond),
        pClient_(nullptr),
        ignorePlebs_(false)
{
    strcpy(channel_, pChannel);
    secondTimer_.async_wait([this](const bs::error_code& ec) {second_timer_elapsed(ec); });
}

channel_context::~channel_context()
{
}

void channel_context::stop()
{
    secondTimer_.cancel();
}

void channel_context::process_message(irc_message_data* pMessageData)
{
    if (pMessageData)
    {
        tp_queue_work([this, pMessageData]() { process_message_core(pMessageData); });
    }
}

user_access_level channel_context::get_user_access_level(irc_message_data* pContext) const
{
    if (strcmp(pContext->username, DevUsername) == 0) return user_access_level::dev;
    if (strcmp(pContext->username, BroadcasterUsername) == 0) return user_access_level::broadcaster;
    if (pContext->isMod) return user_access_level::moderator;
    if (pContext->isSub) return user_access_level::subscriber;
    // todo: regular check here
    return user_access_level::pleb;
}

void channel_context::send_message(send_type type, const char* format, ...) const
{
    char* pBuffer = get_string();
    
    va_list va;
    va_start(va, format);
    vsnprintf(pBuffer, ASHBOT_STRING_BUFFER_SIZE, format, va);
    va_end(va);

    pClient_->send_message(type, channel_, pBuffer);
    release_string(pBuffer);
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

void channel_context::process_message_core(irc_message_data* pMessageData)
{
    command_ptr command = parse_message(pMessageData);

    if (command) command->execute();

    pMessageData->release();
}

command_ptr channel_context::parse_message(irc_message_data* pMessageData)
{
    if (pMessageData->message[0] != command_prefix) return command_factory::null_command;

    const char* pKeyword = pMessageData->message + 1;

    const char* space = strchr(pKeyword, ' ');
    size_t keywordLength = space ? space - pKeyword : strlen(pKeyword);

    user_access_level accessLevel = get_user_access_level(pMessageData);
    if (accessLevel == user_access_level::pleb && ignorePlebs_) return command_factory::null_command;
    if (is_user_ignored(pMessageData->username)) return command_factory::null_command;

    char* _pKeyword = static_cast<char*>(alloca(keywordLength + 1)); // mind the \0
    strncpy(_pKeyword, pKeyword, keywordLength);
    _pKeyword[keywordLength] = 0;

    command_ptr command = commandFactory_.create_command(_pKeyword);
    if (!command) return command; // pretty much same thing as null_command
    if (command->accessible_by(accessLevel))
    {
        command->set_params(this, pMessageData->username);
        command->set_message(pMessageData->message);
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
    if (strcmp(pContext->username, DevUsername) == 0 || strcmp(pContext->username, BroadcasterUsername) == 0)
    {
        return true;
    }

    boost::upgrade_lock<boost::shared_mutex> sl(cdMutex_);
    
    command_clock::time_point lastUse = cooldownMap_[command->id()];
    if (command_clock::now() - lastUse < command->cooldown_)
    {
        return !command->restrictMods_ && pContext->isMod;
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> el(sl);
    cooldownMap_[command->id()] = command_clock::now();
    return true;
}
}
