#include "string_pool.h"
#include "irc/irc_message_data.h"

namespace ashbot {
namespace globals {

std::unique_ptr<string_pool> g_stringPool = std::make_unique<string_pool>();
std::unique_ptr<message_data_pool> g_messageDataPool = std::make_unique<message_data_pool>();

}
}
