#pragma once

// Which graphics backend drives the renderer. Desktop ships OpenGL today;
// the others exist so the seams are proven before the first port starts.
enum class GraphicsApi {
    OpenGL,     // current desktop backend (Windows/Linux/macOS)
    Vulkan,     // planned: Windows/Linux/Android
    Metal,      // planned: macOS/iOS
    MobileGLES, // planned: Android/iOS bring-up path (GLES3 / ANGLE)
};

inline const char* graphicsApiName(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::OpenGL: return "OpenGL";
    case GraphicsApi::Vulkan: return "Vulkan";
    case GraphicsApi::Metal: return "Metal";
    case GraphicsApi::MobileGLES: return "Mobile GLES";
    }
    return "unknown";
}
