#include "core/paths.h"
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

static std::string EnsureDir(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);   // best effort; callers handle IO failure
    return path;
}

std::string UserDataDir() {
    static std::string cached;
    if (!cached.empty()) return cached;

#if defined(_WIN32)
    const char* base = getenv("APPDATA");
    cached = base ? std::string(base) + "/Zion" : std::string("userdata");
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    cached = home ? std::string(home) + "/Library/Application Support/Zion"
                  : std::string("userdata");
#else
    const char* home = getenv("HOME");
    cached = home ? std::string(home) + "/.local/share/zion" : std::string("userdata");
#endif
    return EnsureDir(cached);
}

std::string SettingsFilePath() { return UserDataDir() + "/settings.json"; }
std::string SavesDir()         { return EnsureDir(UserDataDir() + "/saves"); }
std::string SaveSlotPath(int slot) { return SavesDir() + "/slot" + std::to_string(slot) + ".json"; }
std::string LogFilePath()      { return UserDataDir() + "/zion.log"; }
