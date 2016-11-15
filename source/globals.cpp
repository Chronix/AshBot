#include "string_pool.h"
#include "irc/irc_message_data.h"

namespace ashbot {
namespace globals {

std::unique_ptr<string_pool> g_stringPool = std::make_unique<string_pool>();

}
}
