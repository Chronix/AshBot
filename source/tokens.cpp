#include "tokens.h"
#include "db/db.h"

namespace ashbot {
namespace tokens {

namespace {

enum
{
    TOKEN_TWITCH_SECRET = 0,
    TOKEN_YOUTUBE_UNUSED = 1,
    TOKEN_YOUTUBE = 2,
    TOKEN_TWITCH_USER = 3,
    TOKEN_SOUNDCLOUD_ID = 4,
    TOKEN_SOUNDCLOUD_SECRET = 5,
    TOKEN_TWITCH_CHAT = 6,
    TOKEN_VIMEO = 7,
    TOKEN_TWITCH_CLIENT_ID = 8,
    TOKEN_TWITCH_SECRET_DEBUG = 9,
    TOKEN_TWITCH_CLIENT_ID_DEBUG = 10,
    TOKEN_SNAPCHAT_NAME = 11,

    TOKEN_COUNT
};

constexpr char SQL_TOKENS[] = "SELECT type, value FROM tokens;";

std::string Tokens[TOKEN_COUNT];

const char* get_token(int token)
{
    return Tokens[token].c_str();
}

}

bool detail::init_tokens()
{
    db_result result = db::get().query(SQL_TOKENS);
    if (!result) return false;

    char token[ASHBOT_STRING_BUFFER_SIZE];

    for (int i = 0; i < result.row_count(); ++i)
    {
        int16_t tokenType;
        result.read_column(tokenType, i, 0);
        result.read_column(token, i, 1);

        Tokens[tokenType].assign(token);
    }

    return true;
}

const char* youtube()
{
    return get_token(TOKEN_YOUTUBE);
}

const char* twitch_secret()
{
    return get_token(TOKEN_TWITCH_SECRET);
}

const char* twitch_client_id()
{
    return get_token(TOKEN_TWITCH_CLIENT_ID);
}

const char* soundcloud()
{
    return get_token(TOKEN_SOUNDCLOUD_ID);
}

const char* twitch_chat()
{
    return get_token(TOKEN_TWITCH_CHAT);
}

const char* twitch_user()
{
    return get_token(TOKEN_TWITCH_USER);
}

const char* vimeo()
{
    return get_token(TOKEN_VIMEO);
}

const char* snapchat_name()
{
    return get_token(TOKEN_SNAPCHAT_NAME);
}

}
}