#pragma once

#include <atomic>

#include "context_object.h"
#include "mutex.h"

namespace ashbot {

class irc_message_data;
class channel_context;

namespace modules {

class bot_module : public context_object
{
public:
    explicit            bot_module(channel_context* pcc, bool enabled = true) : context_object(pcc), enabled_(enabled) {}
    virtual             ~bot_module() {}
public:
    virtual bool        process_message(irc_message_data* pData) { return true; }
    virtual bool        enabled() const { return enabled_; }
    virtual void        set_enabled(bool enabled) { enabled_ = enabled; }
protected:
    mutex               mutex_;
    std::atomic_bool    enabled_;
};

class timed_module : public bot_module
{
    friend class channel_context;
public:
                        timed_module(channel_context* pcc, int timerIntervalSeconds, bool asyncTimerHandler, bool enabled = true);
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
