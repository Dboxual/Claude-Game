#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

// Pure-data frame description. The game fills one of these per frame; a
// renderer backend consumes it. Nothing in here references any graphics API,
// which is what keeps gameplay portable.

struct BoxDraw {
    glm::vec3 center{0.0f};
    glm::vec3 size{1.0f};
    glm::vec3 color{1.0f};
    bool checkerTop = false;
};

// Oriented box in CAMERA space (+X right, +Y up, -Z forward) for the
// first-person viewmodel. Backends draw these after the world with the
// depth buffer cleared and view = identity, projected through
// RenderFrame::viewmodelProj - so the held weapon never clips into walls
// and keeps its size regardless of the world FOV setting.
struct ViewmodelBoxDraw {
    glm::mat4 transform{1.0f}; // full model matrix (translate*rotate*scale)
    glm::vec3 color{1.0f};
    float emissive = 0.0f; // 1 = unlit/full-bright (muzzle flash)
};

struct TextDraw {
    float x = 0.0f; // pixels from top-left; the center X when centered==true
    float y = 0.0f;
    float scale = 1.0f;
    glm::vec4 color{1.0f};
    bool centered = false;
    std::string text;
};

// Screen-space filled rectangle (menus, overlays). Pixels from top-left.
struct RectDraw {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    glm::vec4 color{1.0f};
};

struct RenderFrame {
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::mat4 viewmodelProj{1.0f}; // fixed-FOV projection for viewmodelBoxes
    int viewportW = 0;
    int viewportH = 0;
    std::vector<BoxDraw> boxes;
    std::vector<ViewmodelBoxDraw> viewmodelBoxes; // first-person hands/weapon
    std::vector<RectDraw> rects; // drawn after the world, before texts
    std::vector<TextDraw> texts;
};
