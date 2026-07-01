#include "platform/sdl/sdl_filesystem.h"

#include <SDL3/SDL.h>

std::optional<std::string> SdlFileSystem::readTextFile(const std::string& path) const {
    size_t size = 0;
    void* data = SDL_LoadFile(path.c_str(), &size);
    if (!data) return std::nullopt;
    std::string text(static_cast<const char*>(data), size);
    SDL_free(data);
    return text;
}

bool SdlFileSystem::exists(const std::string& path) const {
    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
    if (!io) return false;
    SDL_CloseIO(io);
    return true;
}

std::string SdlFileSystem::basePath() const {
    const char* base = SDL_GetBasePath();
    return base ? base : "";
}
