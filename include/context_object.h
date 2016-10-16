#pragma once

#include <stdarg.h>

#include <functional>

namespace ashbot {

template<typename _TCtx>
class context_object
{
protected:
    explicit context_object(_TCtx ctx) : context_(ctx) {}
protected:
    void    send_message(const char* format, bool action, ...)
    {
        static constexpr auto vsend_message = &std::remove_pointer_t<std::decay_t<_TCtx>>::vsend_message;

        va_list va;
        va_start(va, format);
        std::invoke(vsend_message, context_, format, va, action);
        va_end(va);
    }
protected:
    _TCtx   context_;
};

#define AshBotSendMessage(format, ...) send_message(format, false, __VA_ARGS__)
#define AshBotSendAction(format, ...) send_message(format, true, __VA_ARGS__)

}
