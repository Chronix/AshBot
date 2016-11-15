#include "modules/bot_module.h"
#include "channel_context.h"

namespace ashbot {
namespace modules {

timed_module::timed_module(channel_context* pcc, int timerIntervalSeconds, bool asyncTimerHandler, bool enabled)
    :   bot_module(pcc, enabled)
    ,   timerHitCount_(0)
    ,   timerIntervalSeconds_(timerIntervalSeconds)
    ,   asyncTimerHandler_(asyncTimerHandler)
{
}

void timed_module::timer_second_elapsed()
{
    ++timerHitCount_;
    
    if (timerHitCount_ >= timerIntervalSeconds_)
    {
        if (asyncTimerHandler_) context()->execute_async([this]() { timer_elapsed(); });
        else timer_elapsed();

        timerHitCount_ = 0;
    }
}
}
}
