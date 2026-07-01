#include "engine/log.h"

#include <cstdarg>
#include <cstdio>

namespace eng {

namespace {
void logV(FILE* stream, const char* prefix, const char* fmt, va_list args) {
    std::fputs(prefix, stream);
    std::vfprintf(stream, fmt, args);
    std::fputc('\n', stream);
    std::fflush(stream);
}
} // namespace

void logInfo(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logV(stdout, "[info] ", fmt, args);
    va_end(args);
}

void logError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logV(stderr, "[error] ", fmt, args);
    va_end(args);
}

} // namespace eng
