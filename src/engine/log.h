#pragma once

// The single logging seam for every layer. Gameplay and renderer code log
// through this instead of a platform API, so they stay portable; a future
// platform (Android logcat, console SDKs) only has to redirect these two.
namespace eng {

void logInfo(const char* fmt, ...);
void logError(const char* fmt, ...);

} // namespace eng
