#include "irc/irc_connection.h"
#include "irc/irc_enums.h"
#include "logging.h"
#include "thread_pool.h"

namespace ashbot {

namespace {

const std::string IrcServer("irc.chat.twitch.tv");
const std::string IrcPort("6667");

const std::string IRC_DELIMITER("\r\n");

}

namespace ph = baio::placeholders;

irc_connection::irc_connection()
    :   socket_(tp_get_ioservice())
    ,   connected_(false)
    ,   registered_(false)
    ,   pingTimeout_(300)
    ,   pingInterval_(150)
    ,   pingTimer_(tp_get_ioservice(), boost::posix_time::seconds(pingInterval_))
    ,   pingLineLength_(0)
{
    time_point now = clock_type::now();
    lastPingSent_ = now;
    lastPongReceived_ = now;
}

irc_connection::~irc_connection()
{
}

bool irc_connection::connect()
{
    if (connected_)
    {
        AshBotLogError << "Already connected!";
        return false;
    }

    tcp::resolver resolver(tp_get_ioservice());
    tcp::resolver::query query(IrcServer, IrcPort);
    bs::error_code ec;

    auto rIt = resolver.resolve(query, ec);
    decltype(rIt) end;

    if (ec)
    {
        AshBotLogError << "Could not resolve " << IrcServer << " [" << ec.message() << "]";
        return false;
    }

    while (rIt != end)
    {
        socket_.close();
        AshBotLogInfo << "Connecting to " << rIt->endpoint().address().to_string();
        socket_.connect(*rIt, ec);

        if (!ec)
        {
            socket_.set_option(tcp::no_delay(true));

            AshBotLogInfo << "Connected to " << IrcServer;

            pingLineLength_ = sprintf(pingLine_, "PING %s\r\n", IrcServer.c_str());
            connected_ = true;
            pingTimer_.async_wait([this](const bs::error_code& ec) { ping_timer_elapsed(ec); });
            read_line();

            return true;
        }
    }

    AshBotLogError << "Failed to connect!";
    return false;
}

void irc_connection::disconnect()
{
    if (!connected_)
    {
        AshBotLogError << "Cannot disconnect when not connected";
        return;
    }

    AshBotLogInfo << "Disconnecting...";

    bs::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);

    if (ec) AshBotLogWarn << "Socket shutdown failed: " << ec.message();

    socket_.close(ec);
    connected_ = false;
    registered_ = false;
    AshBotLogInfo << "Disconnected...";
}

void irc_connection::reconnect()
{
    AshBotLogInfo << "Reconnecting...";
    disconnect();
    connect();
}

void irc_connection::stop()
{
    if (connected_) disconnect();
    pingTimer_.cancel();
}

void irc_connection::write_line(char* pLine, int lineLength, bool async)
{
    if (!connected_)
    {
        AshBotLogError << "Line '" << pLine << "' cannot be written - not connected!";
        return;
    }

    size_t dataSize = lineLength < 0 ? strlen(pLine) : static_cast<size_t>(lineLength);
    char* pEnd = pLine + dataSize;
    *pEnd++ = '\r';
    *pEnd = '\n';
    dataSize += 2;

    if (async)
    {
        auto handler = [this, pLine](const bs::error_code& ec, size_t transferred)
        {
            line_written(ec, transferred);
            release_line_buffer(pLine);
        };

        async_write(socket_, baio::const_buffers_1(pLine, dataSize), std::move(handler));
    }
    else
    {
        bs::error_code ec;
        write(socket_, baio::const_buffers_1(pLine, dataSize), ec);
        release_line_buffer(pLine);
        if (ec) AshBotLogError << "Sync write_line failed: " << ec.message();
    }
}

void irc_connection::write_hardcoded_line(const char* line, int lineLength, bool async)
{
    if (async)
    {
        async_write(socket_, baio::const_buffers_1(line, lineLength),
                    [this](const bs::error_code& ec, size_t s) { line_written(ec, s); });
    }
    else
    {
        bs::error_code ec;
        write(socket_, baio::const_buffers_1(line, lineLength), ec);
        if (ec) AshBotLogError << "Sync write_hardcoded_line failed: " << ec.message();
    }
}

char* irc_connection::get_line_buffer()
{
    return linePool_.get_block();
}

void irc_connection::release_line_buffer(char* pBuffer)
{
    linePool_.release_block(pBuffer);
}

void irc_connection::read_line()
{
    async_read_until(socket_, buffer_, IRC_DELIMITER,
                     [this](const bs::error_code& ec, size_t s) { line_read(ec, s); });
}

void irc_connection::line_read(const bs::error_code& ec, size_t bytesTransferred)
{
    if (ec)
    {
        if (ec.value() == baio::error::operation_aborted) return;
        
        AshBotLogError << "Async read failed: " << ec.message();
        reconnect(); // assume we disconnected, this may need improvement
    }
    else
    {
        char* pEvLine = get_line_buffer();
        get_line(pEvLine);

        AshBotLogDebug << "Irc Raw Message: " << pEvLine;
        parse_line(pEvLine);
        ev_line_read(pEvLine);
        release_line_buffer(pEvLine);

        read_line();
    }
}

void irc_connection::line_written(const bs::error_code& ec, size_t bytesTransferred)
{
    if (ec) AshBotLogError << "Async write failed: " << ec.message();
}

void irc_connection::ping_timer_elapsed(const bs::error_code& ec)
{
    if (ec)
    {
        if (ec.value() == baio::error::operation_aborted) return;
        AshBotLogError << "Ping timer error: " << ec.message();
    }

    check_ping();

    bs::error_code error;
    pingTimer_.expires_from_now(boost::posix_time::seconds(pingInterval_), error);
    if (ec)
    {
        AshBotLogFatal << "Cannot reschedule ping timer: " << ec.message();
        // abort? we don't really need to ping since the server is already pinging us
    }
    else pingTimer_.async_wait([this](const bs::error_code& ec) { ping_timer_elapsed(ec); });
}

void irc_connection::get_line(char* pBuffer)
{
    int index = 0;

    while (true)
    {
        int c = buffer_.sbumpc();

        if (c == '\r' && buffer_.sgetc() == '\n')
        {
            buffer_.sbumpc();
            break;
        }

        pBuffer[index++] = c;

        // maybe add length check
    }

    pBuffer[index] = 0;
}

void irc_connection::check_ping()
{
    using seconds_t = std::chrono::seconds;

    if (!registered_) return;

    time_point now = clock_type::now();
    seconds_t lastPingSent = std::chrono::duration_cast<seconds_t>(now - lastPingSent_);
    seconds_t lastPongReceived = std::chrono::duration_cast<seconds_t>(now - lastPongReceived_);

    if (lastPingSent.count() < pingTimeout_)
    {
        if (lastPingSent_ > lastPongReceived_) return;
        if (lastPongReceived.count() > pingInterval_)
        {
            // avoid write_line since we are not using a buffer from the line pool
            // we also want specialized write handler to set the last-ping-sent time
            auto handler = [this](const bs::error_code& ec, size_t transferred)
            {
                if (!ec) lastPingSent_ = clock_type::now();
                else if (ec.value() != baio::error::operation_aborted)
                {
                    AshBotLogError << "Failed to send PING: " << ec.message();
                }
            };

            async_write(socket_, baio::const_buffers_1(pingLine_, pingLineLength_), std::move(handler));
        }
    }
    else AshBotLogWarn << "Ping timeout, connection lost";
    // we don't really need to do anything here because if the connection
    // actually IS lost, read_line will fail and attempt to reconnect
}

void irc_connection::parse_line(const char* pRawLine)
{
    const char* pLine;

    if (pRawLine[0] == ':') pLine = strchr(pRawLine, ' ') + 1;
    else pLine = pRawLine;

    if (!registered_)
    {
        char* pEnd;
        int cmdValue = strtol(pLine, &pEnd, 10);

        if (*pEnd == ' ')
        {
            reply_code rc = static_cast<reply_code>(cmdValue);

            if (rc == reply_code::welcome)
            {
                registered_ = true;
                AshBotLogInfo << "Logged in...";
                ev_logged_in();
            }
        }
    }
    
    if (strncmp(pLine, "PONG ", 5) == 0) lastPongReceived_ = clock_type::now();
}

}
