#include "core/log.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>

static FILE* s_file = nullptr;

void LogOpenFile(const std::string& path) {
    if (s_file) return;
    s_file = fopen(path.c_str(), "w");
}

void LogClose() {
    if (s_file) { fclose(s_file); s_file = nullptr; }
}

void LogPrintf(LogLevel lvl, const char* fmt, ...) {
    const char* tag = lvl == LogLevel::Info ? "INFO" : lvl == LogLevel::Warn ? "WARN" : "ERROR";

    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    time_t now = time(nullptr);
    tm* t = localtime(&now);
    char stamp[16];
    strftime(stamp, sizeof(stamp), "%H:%M:%S", t);

    printf("[%s %s] %s\n", stamp, tag, msg);
    if (s_file) {
        fprintf(s_file, "[%s %s] %s\n", stamp, tag, msg);
        fflush(s_file);   // crash-safe: the last line is the one you need
    }
}
