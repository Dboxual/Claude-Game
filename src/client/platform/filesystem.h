#pragma once

#include <optional>
#include <string>
#include <vector>

// File access seam. Desktop reads from disk; Android will read from APK
// assets and consoles from their packaged filesystems, so gameplay must
// never open files directly.
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual std::optional<std::string> readTextFile(const std::string& path) const = 0;
    virtual bool writeTextFile(const std::string& path, const std::string& text) const = 0;
    virtual bool exists(const std::string& path) const = 0;

    // Creates the directory and any missing parents; true if it exists after.
    virtual bool createDirectory(const std::string& path) const = 0;

    // Names of the entries directly inside path (no dot entries, no
    // recursion). Empty if the directory does not exist.
    virtual std::vector<std::string> listDirectory(const std::string& path) const = 0;

    // Directory of the executable / bundled resources, with a trailing
    // separator. Empty if the platform cannot provide one.
    virtual std::string basePath() const = 0;
};
