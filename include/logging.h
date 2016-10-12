#pragma once

#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>

namespace ashbot {
    
void log_cleanup();

BOOST_LOG_GLOBAL_LOGGER(__logger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>);

#define AshBot_LogMessage(severity) BOOST_LOG_SEV(ashbot::__logger::get(), boost::log::trivial::severity) << __FUNCTION__ << ": "

#define AshBotLogFatal   AshBot_LogMessage(fatal)
#define AshBotLogError   AshBot_LogMessage(error)
#define AshBotLogWarn    AshBot_LogMessage(warning)
#define AshBotLogInfo    AshBot_LogMessage(info)
#define AshBotLogDebug   AshBot_LogMessage(debug)

}
