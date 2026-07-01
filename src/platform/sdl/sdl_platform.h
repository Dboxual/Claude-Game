#pragma once

#include "platform/platform.h"

// Desktop platform factory (Windows/Linux/macOS via SDL3). This header
// deliberately exposes no SDL types so the composition root stays portable.
PlatformSystems createSdlPlatform(const WindowDesc& desc);

// Tears down the systems in the right order and shuts SDL down.
void destroySdlPlatform(PlatformSystems& systems);
