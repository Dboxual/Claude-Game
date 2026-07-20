// Per-user data locations. Settings/saves/logs never live next to the exe:
// Windows -> %APPDATA%/Zion, macOS -> ~/Library/Application Support/Zion,
// fallback -> ./userdata (portable mode).
#pragma once
#include <string>

std::string UserDataDir();                    // created on first call
std::string SettingsFilePath();               // <data>/settings.json
std::string SavesDir();                       // <data>/saves (created)
std::string SaveSlotPath(int slot);           // <data>/saves/slot<N>.json
std::string LogFilePath();                    // <data>/zion.log
