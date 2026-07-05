#pragma once

#include "client/platform/filesystem.h"

// Desktop filesystem via SDL's IO layer. Using SDL_LoadFile (rather than
// std::ifstream) means the same code path will read Android APK assets when
// that platform lands.
class SdlFileSystem final : public IFileSystem {
public:
    std::optional<std::string> readTextFile(const std::string& path) const override;
    bool writeTextFile(const std::string& path, const std::string& text) const override;
    bool exists(const std::string& path) const override;
    bool createDirectory(const std::string& path) const override;
    std::vector<std::string> listDirectory(const std::string& path) const override;
    std::string basePath() const override;
};
