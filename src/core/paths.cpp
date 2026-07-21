#include "core/paths.h"
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

static std::string dataOverride;
static std::string cachedDataDir;

static std::string EnsureDir(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);   // best effort; callers handle IO failure
    return path;
}

void SetUserDataOverride(const std::string& path) {
    // UserDataDir is queried only after argument parsing. Ignore late changes
    // so every subsystem agrees on one directory for the whole process.
    if (cachedDataDir.empty()) dataOverride = path;
}

std::string UserDataDir() {
    if (!cachedDataDir.empty()) return cachedDataDir;
    if (!dataOverride.empty()) {
        cachedDataDir = dataOverride;
        return EnsureDir(cachedDataDir);
    }

#if defined(_WIN32)
    const char* base = getenv("APPDATA");
    cachedDataDir = base ? std::string(base) + "/Zion" : std::string("userdata");
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    cachedDataDir = home ? std::string(home) + "/Library/Application Support/Zion"
                         : std::string("userdata");
#else
    const char* home = getenv("HOME");
    cachedDataDir = home ? std::string(home) + "/.local/share/zion" : std::string("userdata");
#endif
    return EnsureDir(cachedDataDir);
}

std::string SettingsFilePath() { return UserDataDir() + "/settings.json"; }
std::string SavesDir()         { return EnsureDir(UserDataDir() + "/saves"); }
std::string SaveSlotPath(int slot) { return SavesDir() + "/slot" + std::to_string(slot) + ".json"; }
std::string LogFilePath()      { return UserDataDir() + "/zion.log"; }
