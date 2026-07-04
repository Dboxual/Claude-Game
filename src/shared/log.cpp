#include "shared/log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace eng {

namespace {
void logV(FILE* stream, const char* prefix, const char* fmt, va_list args) {
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char stamp[16];
    std::strftime(stamp, sizeof(stamp), "%H:%M:%S", &tmv);
    std::fprintf(stream, "[%s] ", stamp);
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
