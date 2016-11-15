#include <stdarg.h>

#include "context_object.h"
#include "channel_context.h"

namespace ashbot {

void context_object::send_message(const char* format, bool action, ...) const
{
    va_list va;
    va_start(va, action);
    context_->vsend_message(format, va, action);
    va_end(va);
}

}
