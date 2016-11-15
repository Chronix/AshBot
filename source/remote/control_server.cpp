#include "remote/control_server.h"

#include <jsoncons/json.hpp>

#include "modules/songrequest/songrequest_provider.h"
#include "thread_pool.h"

namespace ashbot {

namespace bs = boost::system;
namespace jc = jsoncons;

control_server::control_server(channel_context& context)
    :   context_(context)
{
    using namespace websocketpp;
    
    bs::error_code ec;
    endpoint_.init_asio(&detail::Ios, ec);
    if (ec)
    {
        AshBotLogError << "Failed to initialize ASIO: " << ec.message();
    }

    endpoint_.set_message_handler([this](connection_hdl c, ws_server::message_ptr m) { on_message(c, m); });
}

void control_server::on_message(ws::connection_hdl connection, ws_server::message_ptr message) const
{
    jc::json jsonData = jc::json::parse(message->get_payload());
    std::string command(jsonData["command"].as_string());

    if (command == "srstate")
    {
        int64_t songId = jsonData["songid"].as_integer();
        bool isPlaying = jsonData["playing"].as_bool();
        context_.songrequest_provider()->song_state_change(songId, isPlaying);
    }
}
}
