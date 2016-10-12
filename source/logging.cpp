#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/core/null_deleter.hpp>

#include "logging.h"

namespace ashbot {

namespace bl = boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", boost::log::trivial::severity_level)

void log_cleanup()
{
    bl::core::get()->remove_all_sinks();
}

BOOST_LOG_GLOBAL_LOGGER_INIT(__logger, bl::sources::severity_logger_mt)
{
    bl::sources::severity_logger_mt<boost::log::trivial::severity_level> logger;
    logger.add_attribute("TimeStamp", bl::attributes::local_clock());

    using text_sink = bl::sinks::asynchronous_sink<bl::sinks::text_ostream_backend>;
    auto consoleSink = boost::make_shared<text_sink>();
    consoleSink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter()));
    consoleSink->set_filter(severity >= boost::log::trivial::severity_level::warning);

    using file_sink = bl::sinks::asynchronous_sink<bl::sinks::text_file_backend>;
    auto logfileSink = boost::make_shared<file_sink>();
    
    {
        auto logfileBackend = logfileSink->locked_backend();
        logfileBackend->set_time_based_rotation(bl::sinks::file::rotation_at_time_point(0, 0, 0));
        logfileBackend->set_file_name_pattern("Logs\\ashbot_%Y%m%d.log");
        logfileBackend->auto_flush();
        logfileBackend->set_file_collector(
            bl::sinks::file::make_collector(bl::keywords::target = "Logs\\Archive")
        );
    }

    namespace expr = bl::expressions;

    bl::formatter fmt = expr::stream
        << format_date_time(timestamp, "%Y-%m-%d %H:%M:%S.%f")
        << " [" << boost::log::trivial::severity << "] "
        << expr::smessage;
    logfileSink->set_formatter(fmt);

    bl::core::get()->add_sink(consoleSink);
    bl::core::get()->add_sink(logfileSink);

    return logger;
}

}
