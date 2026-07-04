#include "client/renderer/renderer.h"

#include "client/renderer/metal/metal_renderer.h"
#include "client/renderer/mobile/gles_renderer.h"
#include "client/renderer/opengl/gl_renderer.h"
#include "client/renderer/vulkan/vulkan_renderer.h"

std::unique_ptr<IRenderer> createRenderer(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::OpenGL: return std::make_unique<GLRenderer>();
    case GraphicsApi::Vulkan: return std::make_unique<VulkanRenderer>();
    case GraphicsApi::Metal: return std::make_unique<MetalRenderer>();
    case GraphicsApi::MobileGLES: return std::make_unique<GlesRenderer>();
    }
    return nullptr;
}
