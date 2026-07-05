#include "client/platform/sdl/sdl_filesystem.h"

#include <SDL3/SDL.h>

std::optional<std::string> SdlFileSystem::readTextFile(const std::string& path) const {
    size_t size = 0;
    void* data = SDL_LoadFile(path.c_str(), &size);
    if (!data) return std::nullopt;
    std::string text(static_cast<const char*>(data), size);
    SDL_free(data);
    return text;
}

bool SdlFileSystem::writeTextFile(const std::string& path, const std::string& text) const {
    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "wb");
    if (!io) return false;
    size_t written = SDL_WriteIO(io, text.data(), text.size());
    bool closed = SDL_CloseIO(io);
    return closed && written == text.size();
}

bool SdlFileSystem::exists(const std::string& path) const {
    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
    if (!io) return false;
    SDL_CloseIO(io);
    return true;
}

bool SdlFileSystem::createDirectory(const std::string& path) const {
    return SDL_CreateDirectory(path.c_str());
}

std::vector<std::string> SdlFileSystem::listDirectory(const std::string& path) const {
    std::vector<std::string> names;
    int count = 0;
    char** entries = SDL_GlobDirectory(path.c_str(), nullptr, 0, &count);
    if (!entries) return names;
    names.reserve(size_t(count));
    for (int i = 0; i < count; ++i) names.emplace_back(entries[i]);
    SDL_free(entries);
    return names;
}

std::string SdlFileSystem::basePath() const {
    const char* base = SDL_GetBasePath();
    return base ? base : "";
}
