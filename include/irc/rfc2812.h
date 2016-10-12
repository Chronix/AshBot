#pragma once

#include "irc_connection.h"

namespace ashbot {
namespace rfc2812 {

// note the use of snprintf, NOT _snprintf
#define rfc_format(...) snprintf(pLine, irc_connection::line_pool::BlockSize, Format, __VA_ARGS__)
#define rfc_fun_1(name, format, arg) \
    inline int name##(char* pLine, const char* arg) \
    { static const char* Format = format; return rfc_format(arg); }

#define rfc_fun_2(name, format, arg1, arg2) \
    inline int name##(char* pLine, const char* arg1, const char* arg2) \
    { static const char* Format = format; return rfc_format(arg1, arg2); }

rfc_fun_1(pass, "PASS %s", pPassword)
rfc_fun_1(nick, "NICK %s", pNickname)
rfc_fun_1(join, "JOIN %s", pChannel)
rfc_fun_1(part, "PART %s", pChannel)

rfc_fun_2(oper, "OPER %s %s", pName, pPassword)
rfc_fun_2(privmsg, "PRIVMSG %s :%s", pDestination, pMessage)
rfc_fun_2(notice, "NOTICE %s :%s", pDestination, pMessage)

inline int user(char* pLine, const char* pUsername, int usermode, const char* pRealname)
{
    static const char* Format = "USER %s %i * :%s";
    return rfc_format(pUsername, usermode, pRealname);
}

namespace ext {
    
rfc_fun_2(action, "PRIVMSG %s :\x1" "ACTION %s\x1", pDestination, pMessage)

}

#undef rfc_format
#undef rfc_fun_1
#undef rfc_fun_2

}
}
