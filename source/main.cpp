#include <vld.h>

#include <curl/curl.h>

#include "bot.h"

#include "sub_manager.h"
#include "thread_pool.h"
#include "tokens.h"

int main(int argc, char* argv[])
{
    using namespace ashbot;

    curl_global_init(CURL_GLOBAL_ALL);
    tp_initialize(64);
    tokens::detail::init_tokens();
    sub_manager::initialize();
    sub_manager::get_subs(true);

    bot bot;
    bot.run();

    // todo: do something more productive here
    getchar();
    bot.stop();

    sub_manager::cleanup();
    tp_cleanup();

    log_cleanup();

    curl_global_cleanup();
    return 0;
}
