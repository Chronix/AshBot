#pragma once

#include "channel_context.h"
#include "websocketpp.h"

namespace ashbot {

namespace ws = websocketpp;

class control_server
{
    using ws_server = ws::server<ws::config::asio_tls>;
public:
    explicit control_server(channel_context& context);
private:
    void                on_message(ws::connection_hdl connection, ws_server::message_ptr message) const;
private:
    ws_server           endpoint_;
    channel_context&    context_;
};

}
