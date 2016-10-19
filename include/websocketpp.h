#pragma once

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4267) // conversion from x to y, possible loss of data
#endif

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>

#ifdef _MSC_VER
# pragma warning(pop)
#endif
