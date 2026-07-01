#pragma once

#include "platform/audio.h"
#include "platform/filesystem.h"
#include "platform/input.h"
#include "platform/window.h"

#include <memory>

// Everything platform-specific the engine needs, bundled by a platform
// factory (see platform/sdl/sdl_platform.h for the desktop one). A future
// Android/iOS/console target provides its own factory returning the same
// interfaces.
struct PlatformSystems {
    std::unique_ptr<IWindow> window;
    std::unique_ptr<IInput> input;
    std::unique_ptr<IFileSystem> fileSystem;
    std::unique_ptr<IAudio> audio;

    bool valid() const { return window && input && fileSystem && audio; }
};
