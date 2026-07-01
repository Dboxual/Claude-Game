#pragma once

#include <optional>
#include <string>

// File access seam. Desktop reads from disk; Android will read from APK
// assets and consoles from their packaged filesystems, so gameplay must
// never open files directly.
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual std::optional<std::string> readTextFile(const std::string& path) const = 0;
    virtual bool exists(const std::string& path) const = 0;

    // Directory of the executable / bundled resources, with a trailing
    // separator. Empty if the platform cannot provide one.
    virtual std::string basePath() const = 0;
};
