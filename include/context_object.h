#pragma once

namespace ashbot {

class channel_context;

class context_object
{  
protected:
    explicit context_object(channel_context* pCtx) : context_(pCtx) {}
protected:
    void                send_message(const char* format, bool action, ...) const;
    channel_context*    context() const { return context_; }
    void                set_context(channel_context* pcc) { context_ = pcc; }
private:
    channel_context*    context_;
};

#define AshBotSendMessage(format, ...) send_message(format, false, __VA_ARGS__)
#define AshBotSendAction(format, ...) send_message(format, true, __VA_ARGS__)

}
