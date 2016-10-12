#pragma once

#include <chrono>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "string_pool.h"

namespace ashbot {

namespace baio = boost::asio;
namespace bs = boost::system;
    
class irc_connection
{
    using tcp = baio::ip::tcp;
    
    using clock_type = std::chrono::steady_clock; // monotonic clock
    using time_point = clock_type::time_point;
public:
    using line_pool = object_cache<char, ASHBOT_STRING_BUFFER_SIZE>;
public:
    explicit                irc_connection();
                            virtual ~irc_connection();
public:
    bool                    connect();
    void                    disconnect();
    void                    reconnect();

    void                    stop();

    void                    write_line(char* pLine, int lineLength = -1, bool async = true);
protected:
    virtual void            ev_line_read(const char* pLine) = 0;
    virtual void            ev_logged_in() = 0;
    void                    write_hardcoded_line(const char* line, int lineLength, bool async = true);
    char*                   get_line_buffer();
    void                    release_line_buffer(char* pBuffer);
private:
    void                    read_line();

    void                    line_read(const bs::error_code& ec, size_t bytesTransferred);
    void                    line_written(const bs::error_code& ec, size_t bytesTransferred);
    void                    ping_timer_elapsed(const bs::error_code& ec);

    void                    get_line(char* pBuffer);
    void                    check_ping();
    void                    parse_line(const char* pLine);
protected:
    std::string             nick_;
private:
    tcp::socket             socket_;
    baio::streambuf         buffer_;
    boost::thread_group     ioThreads_;

    line_pool               linePool_;

    bool                    connected_;
    bool                    registered_;

    long                    pingTimeout_;
    long                    pingInterval_;
    time_point              lastPingSent_;
    time_point              lastPongReceived_;
    baio::deadline_timer    pingTimer_;
    char                    pingLine_[64];
    size_t                  pingLineLength_;
};

}
