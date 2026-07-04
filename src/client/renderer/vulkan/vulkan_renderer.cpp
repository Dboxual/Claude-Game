#include "client/renderer/vulkan/vulkan_renderer.h"

#include "shared/log.h"

bool VulkanRenderer::init(IWindow&) {
    eng::logError("Vulkan backend is not implemented yet (planned: Windows/Linux/Android). "
                  "Run with --renderer gl for now.");
    return false;
}
