#pragma once

#include "platform/graphics_api.h"
#include "renderer/render_types.h"

#include <memory>

class IWindow;

// Renderer backend interface. Backends own all graphics-API state; the rest
// of the engine only ever sees RenderFrame data going in.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool init(IWindow& window) = 0;
    virtual void shutdown() = 0;

    // Draws the frame and presents it.
    virtual void render(const RenderFrame& frame) = 0;

    virtual const char* name() const = 0;
};

// Factory. Returns a backend for the requested API; stub backends compile
// everywhere but fail init() with a clear message until implemented.
std::unique_ptr<IRenderer> createRenderer(GraphicsApi api);
