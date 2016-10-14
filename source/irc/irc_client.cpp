#include <boost/assign/list_of.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/xpressive/xpressive.hpp>

#include "irc/irc_client.h"
#include "irc/rfc2812.h"
#include "logging.h"
#include "util.h"

namespace ashbot {

namespace ba = boost::assign;
namespace bx = boost::xpressive;
namespace bc = boost::container;

namespace {

namespace regex {

// "^:[^ ]+? ([0-9]{3}) .+$"
const bx::cregex ReplyCode = bx::bos >> ':' >> -+~bx::as_xpr(' ') >> ' ' >> (bx::s1 = bx::repeat<3>(bx::range('0', '9'))) >> ' ' >> +bx::_ >> bx::eos;

// "^:[^ ]+? CAP \\* ACK .*$"
const bx::cregex CapAck = bx::bos >> ':' >> -+~bx::as_xpr(' ') >> ' ' >> "CAP * ACK" >> *bx::_ >> bx::eos;

// "^PING :.*"
const bx::cregex Ping = bx::bos >> "PING :" >> *bx::_;

// "^ERROR :.*"
const bx::cregex Error = bx::bos >> "ERROR :" >> *bx::_;

// "^:.*? PRIVMSG (.).* :\x1" "ACTION .*\x1$"
const bx::cregex Action = bx::bos >> ':' >> -*bx::_ >> " PRIVMSG " >> (bx::s1 = bx::_) >> *bx::_ >> " :\x1" >> "ACTION " >> *bx::_ >> '\x1' >> bx::eos;

// "^:.*? PRIVMSG .* :\x1.*\x1$"
const bx::cregex CtcpRequest = bx::bos >> ':' >> -*bx::_ >> " PRIVMSG " >> *bx::_ >> " :\x1" >> *bx::_ >> '\x1' >> bx::eos;

// "^:.*? PRIVMSG (.).* :.*$"
const bx::cregex Message = bx::bos >> ':' >> -*bx::_ >> " PRIVMSG " >> (bx::s1 = bx::_) >> *bx::_ >> " :" >> *bx::_ >> bx::eos;

// "^:.*? NOTICE .* :\x1.*\x1$"
const bx::cregex CtcpReply = bx::bos >> ':' >> -*bx::_ >> " NOTICE " >> *bx::_ >> " :\x1" >> *bx::_ >> '\x1' >> bx::eos;

// "^:.*? NOTICE (.).* :.*$"
const bx::cregex Notice = bx::bos >> ':' >> -*bx::_ >> " NOTICE " >> (bx::s1 = bx::_) >> *bx::_ >> " :" >> *bx::_ >> bx::eos;

// "^:.*? JOIN .*$"
const bx::cregex Join = bx::bos >> ':' >> -*bx::_ >> " JOIN " >> *bx::_ >> bx::eos;

// "^:.*? NICK .*$"
const bx::cregex Nick = bx::bos >> ':' >> -*bx::_ >> " NICK " >> *bx::_ >> bx::eos;

// "^:.*? PART .*$"
const bx::cregex Part = bx::bos >> ':' >> -*bx::_ >> " PART " >> *bx::_ >> bx::eos;

}

using rc = reply_code;
using rt = receive_type;

const bc::flat_map<rc, rt> ReceiveTypeMap = ba::map_list_of
(rc::welcome, rt::login)(rc::your_host, rt::login)(rc::created, rt::login)
(rc::my_info, rt::login)(rc::bounce, rt::login)
(rc::luser_client, rt::info)(rc::luser_op, rt::info)(rc::luser_unknown, rt::info)
(rc::luser_me, rt::login)(rc::luser_channels, rt::login)
(rc::motd_start, rt::motd)(rc::motd, rt::motd)(rc::end_of_motd, rt::motd)
(rc::names_reply, rt::name)(rc::end_of_names, rt::name)
(rc::who_reply, rt::who)(rc::end_of_who, rt::who)
(rc::list_start, rt::list)(rc::list, rt::list)(rc::list_end, rt::list)
(rc::ban_list, rt::ban_list)(rc::end_of_ban_list, rt::ban_list)
(rc::topic, rt::topic)(rc::no_topic, rt::topic)
(rc::who_is_user, rt::who_is)(rc::who_is_server, rt::who_is)(rc::who_is_operator, rt::who_is)
(rc::who_is_idle, rt::who_is)(rc::who_is_channels, rt::who_is)(rc::end_of_who_is, rt::who_is)
(rc::who_was_user, rt::who_was)(rc::end_of_who_was, rt::who_was)
(rc::user_mode_is, rt::user_mode)
(rc::channel_mode_is, rt::channel_mode);

const std::array<int, 138> IrcReplyCodes =
{
    0, 1, 2, 3, 4, 5, 200, 201, 202, 203, 204, 205, 206, 207, 208,
    209, 210, 211, 212, 219, 221, 234, 235, 242, 243, 251, 252, 253,
    254, 255, 256, 257, 258, 259, 261, 262, 263, 301, 302, 303, 305,
    306, 311, 312, 313, 314, 315, 317, 318, 319, 321, 322, 323, 324,
    325, 331, 332, 341, 342, 346, 347, 348, 349, 351, 352, 353, 364,
    365, 366, 367, 368, 369, 371, 372, 374, 375, 376, 381, 382, 383,
    391, 392, 393, 394, 395, 401, 402, 403, 404, 405, 406, 407, 408,
    409, 411, 412, 413, 414, 415, 421, 422, 423, 424, 431, 432, 433,
    436, 437, 441, 442, 443, 444, 445, 446, 451, 461, 462, 463, 464,
    465, 466, 467, 471, 472, 473, 474, 475, 476, 477, 478, 481, 482,
    483, 484, 485, 491, 501, 502
};

}

irc_client::irc_client(channel_context& pContext)
    :   channelContext_(pContext)
{
}

void irc_client::login(const char* nickname, const char* oauthToken)
{
    char* pPassLine = get_line_buffer();
    int length = rfc2812::pass(pPassLine, oauthToken);
    write_line(pPassLine, length, false);

    char* pNicknameLine = get_line_buffer();
    length = rfc2812::nick(pNicknameLine, nickname);
    write_line(pNicknameLine, length, false);

    char* pUsernameLine = get_line_buffer();
    length = rfc2812::user(pUsernameLine, nickname, 0, nickname);
    write_line(pUsernameLine, length, false);
}

void irc_client::join(const char* channel)
{
    char* pJoinLine = get_line_buffer();
    int length = rfc2812::join(pJoinLine, channel);
    write_line(pJoinLine, length, false);
}

void irc_client::send_message(send_type type, const char* pDestination, const char* pMessage)
{
    int(*formatFn)(char*, const char*, const char*) = nullptr;

    switch (type)
    {
    case send_type::message: formatFn = rfc2812::privmsg; break;
    case send_type::action: formatFn = rfc2812::ext::action; break;
    case send_type::notice: formatFn = rfc2812::notice; break;
    }

    if (formatFn)
    {
        char* pBuffer = get_line_buffer();
        int length = formatFn(pBuffer, pDestination, pMessage);
        write_line(pBuffer, length);
    }
}

void irc_client::ev_line_read(const char* pLine)
{
    irc_message_data* pData = irc_message_data::get();
    parse_message(pData, pLine);
    dispatch_message(pData);
}

void irc_client::ev_logged_in()
{
    static const char TwitchCommandsReq[] = "CAP REQ :twitch.tv/commands\r\n";
    static const char TwitchTagsReq[] = "CAP REQ :twitch.tv/tags\r\n";

    write_hardcoded_line(TwitchCommandsReq, static_strlen(TwitchCommandsReq));
    write_hardcoded_line(TwitchTagsReq, static_strlen(TwitchTagsReq));
}

void irc_client::parse_message(irc_message_data* pData, const char* pLine) const
{
    const char* pRawLine = pLine[0] == '@' ? parse_tags(pData, pLine) : pLine;
    
    reply_code replyCode;
    receive_type type = get_message_type(pRawLine, replyCode);

    if (pRawLine[0] == ':') ++pRawLine;

    const char* pMessage = nullptr;
    const char* colonPos = strstr(pRawLine, " :");
    const char* nickEnd = strpbrk(pRawLine, "! "); // strchr(pRawLine, '!');

    if (*nickEnd == ' ')
    {
        const char* atPos = strpbrk(pRawLine, "@ ");
        if (*atPos == '@') nickEnd = atPos;
        else nickEnd = pRawLine;
    }

    if (!nickEnd) nickEnd = strchr(pRawLine, ' ');
    if (colonPos) pMessage = colonPos + 2;

    if (replyCode == reply_code::null)
    {
        const char* pMessageCode = strchr(pRawLine, ' ');
        if (pMessageCode)
        {
            ++pMessageCode;
            char* endPtr;
            int messageCode = strtol(pMessageCode, &endPtr, 10);
            if (endPtr != pMessageCode) replyCode = static_cast<reply_code>(messageCode);
        }
    }

    size_t nickLength = nickEnd - pRawLine;
    assert(nickLength < (irc_message_data::MAX_USERNAME_LENGTH - 1));
    memcpy(pData->username, pRawLine, nickLength);
    pData->username[nickLength] = 0;
    
    if (pMessage) strcpy(pData->message, pMessage);

    pData->replyCode = replyCode;
    pData->receiveType = type;
}

const char* irc_client::parse_tags(irc_message_data* pData, const char* pLine) const
{
    const char* ircMessageStart = strchr(pLine, ' ') + 1;
    const char* tags = pLine + 1;

    while (tags != ircMessageStart)
    {
        const char* separator = strpbrk(tags, "=; ");
        std::string tagKey(tags, separator - tags);
        unescape_tag_string(tagKey);
        tags = separator + 1;
        
        if (*separator != '=')
        {
            pData->tags[move(tagKey)]; // just insert key with default (= empty string) value
            continue;
        }

        separator = strpbrk(tags, "; ");
        std::string tagValue(tags, separator - tags);
        unescape_tag_string(tagValue);
        tags = separator + 1;

        pData->tags[move(tagKey)] = move(tagValue);
    }

    return ircMessageStart;
}

receive_type irc_client::get_message_type(const char* pRawLine, reply_code& pRc) const
{
    bx::cmatch match;

    if (regex_match(pRawLine, match, regex::ReplyCode))
    {
        char* endPtr;
        int code = strtol(match[1].first, &endPtr, 10);
        assert(endPtr && *endPtr == ' ');

        if (!binary_search(IrcReplyCodes.begin(), IrcReplyCodes.end(), code))
        {
            AshBotLogWarn << "Received unrecognized reply code " << code;
            return receive_type::unknown;
        }

        reply_code rc = static_cast<reply_code>(code);
        pRc = rc;

        if (code >= 400 && code <= 599) return receive_type::error_message;
        auto rtIter = ReceiveTypeMap.find(rc);
        if (rtIter != ReceiveTypeMap.end()) return rtIter->second;
        return receive_type::unknown;
    }

    pRc = reply_code::null;

    if (regex_match(pRawLine, match, regex::Message))
    {
        switch (*match[1].first)
        {
        case '#':
        case '!':
        case '&':
        case '+':
            return receive_type::channel_message;
        default:
            return receive_type::query_message;
        }
    }

    if (regex_match(pRawLine, match, regex::Action))
    {
        switch (*match[1].first)
        {
        case '#':
        case '!':
        case '&':
        case '+':
            return receive_type::channel_action;
        default:
            return receive_type::query_action;
        }
    }

    if (regex_match(pRawLine, match, regex::Join)) return receive_type::join;
    if (regex_match(pRawLine, match, regex::Part)) return receive_type::part;

    if (regex_match(pRawLine, match, regex::Notice))
    {
        switch (*match[1].first)
        {
        case '#':
        case '!':
        case '&':
        case '+':
            return receive_type::channel_notice;
        default:
            return receive_type::query_notice;
        }
    }
    
    if (regex_match(pRawLine, match, regex::Ping)) return receive_type::ping;
    if (regex_match(pRawLine, match, regex::Error)) return receive_type::error;
    if (regex_match(pRawLine, match, regex::CapAck)) return receive_type::cap_ack;
    if (regex_match(pRawLine, match, regex::CtcpRequest)) return receive_type::ctcp_request;
    if (regex_match(pRawLine, match, regex::CtcpReply)) return receive_type::ctcp_reply;
    if (regex_match(pRawLine, match, regex::Nick)) return receive_type::nick_change;

    AshBotLogWarn << "Message type unknown: " << pRawLine;
    return receive_type::unknown;
}

void irc_client::dispatch_message(irc_message_data* pData)
{
    switch (pData->receiveType)
    {
    case receive_type::ping: event_ping(pData); break;
    case receive_type::error: event_error(pData); break;
    case receive_type::channel_message:
    case receive_type::query_message:
        event_message(pData); 
        return; // return, not break
    }

    pData->release();
}

void irc_client::event_ping(irc_message_data* pData)
{
    // hardcode this, very unlikely to change and easy to fix anyway
    static const char PongReply[] = "PONG :tmi.twitch.tv\r\n";
    write_hardcoded_line(PongReply, static_strlen(PongReply));
}

void irc_client::event_error(irc_message_data* pData)
{
    AshBotLogWarn << "IRC Error: " << pData->message;
}

void irc_client::event_message(irc_message_data* pData)
{
    channelContext_.process_message(pData);
}

void irc_client::unescape_tag_string(std::string& ts)
{
    std::string unescaped;
    unescaped.reserve(ts.length());
    size_t lastPos = 0;
    size_t pos;
    const char* sequence;

    while (lastPos < ts.length() && (pos = ts.find('\\', lastPos)) != std::string::npos)
    {
        unescaped.append(&ts[lastPos], pos - lastPos);
        sequence = &ts[pos];

        if (strncmp(sequence, "\\:", 2) == 0) unescaped.push_back(';');
        else if (strncmp(sequence, "\\s", 2) == 0) unescaped.push_back(' ');
        else if (strncmp(sequence, "\\\\", 2) == 0) unescaped.push_back('\\');
        else if (strncmp(sequence, "\\r", 2) == 0) unescaped.push_back('\r');
        else if (strncmp(sequence, "\\n", 2) == 0) unescaped.push_back('\n');

        lastPos = pos + 2;
    }

    if (lastPos < ts.length()) unescaped.append(ts.substr(lastPos));

    ts.swap(unescaped);
}

}
