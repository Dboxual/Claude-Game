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

struct TextDraw {
    float x = 0.0f; // pixels from top-left; the center X when centered==true
    float y = 0.0f;
    float scale = 1.0f;
    glm::vec4 color{1.0f};
    bool centered = false;
    std::string text;
};

struct RenderFrame {
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    int viewportW = 0;
    int viewportH = 0;
    std::vector<BoxDraw> boxes;
    std::vector<TextDraw> texts;
};
