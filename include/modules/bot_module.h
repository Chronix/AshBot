#pragma once

#include "mutex.h"

namespace ashbot {

struct irc_message_data;
class channel_context;

namespace modules {

class bot_module
{
public:
    explicit            bot_module(channel_context& cc) : context_(cc) {}
    virtual             ~bot_module() {}
public:
    virtual bool        process_message(irc_message_data* pData) { return true; }
protected:
    channel_context&    context_;
    mutex               mutex_;
};

class timed_module : public bot_module
{
    friend class channel_context;
public:
                        timed_module(channel_context& cc, int timerIntervalSeconds, bool asyncTimerHandler);
protected:
    virtual void        timer_elapsed() = 0;
private:
    void                timer_second_elapsed();
private:
    int                 timerHitCount_;
    int                 timerIntervalSeconds_;
    bool                asyncTimerHandler_;
};

}

}
