// Minimal logging: console always, plus a session log file in the user data
// dir once LogOpenFile is called. Use the macros; they add level + timestamp.
#pragma once
#include <string>

enum class LogLevel { Info, Warn, Error };

void LogOpenFile(const std::string& path);   // safe to call once, early
void LogClose();
void LogPrintf(LogLevel lvl, const char* fmt, ...);

#define LOG_INFO(...)  LogPrintf(LogLevel::Info,  __VA_ARGS__)
#define LOG_WARN(...)  LogPrintf(LogLevel::Warn,  __VA_ARGS__)
#define LOG_ERROR(...) LogPrintf(LogLevel::Error, __VA_ARGS__)
