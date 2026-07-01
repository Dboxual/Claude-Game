#include "renderer/renderer.h"

#include "renderer/metal/metal_renderer.h"
#include "renderer/mobile/gles_renderer.h"
#include "renderer/opengl/gl_renderer.h"
#include "renderer/vulkan/vulkan_renderer.h"

std::unique_ptr<IRenderer> createRenderer(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::OpenGL: return std::make_unique<GLRenderer>();
    case GraphicsApi::Vulkan: return std::make_unique<VulkanRenderer>();
    case GraphicsApi::Metal: return std::make_unique<MetalRenderer>();
    case GraphicsApi::MobileGLES: return std::make_unique<GlesRenderer>();
    }
    return nullptr;
}
