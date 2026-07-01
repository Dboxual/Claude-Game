#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

// Pixel-perfect HUD text using an embedded 5x7 glyph table (uppercase,
// digits, and common punctuation) baked into a texture atlas at init.
// Coordinates are pixels from the top-left of the window.
class DebugText {
public:
    bool init();
    void shutdown();

    void begin();
    void add(float x, float y, float scale, const glm::vec4& color, const std::string& text);
    void addCentered(float centerX, float y, float scale, const glm::vec4& color, const std::string& text);
    float textWidth(const std::string& text, float scale) const;
    void flush(int screenW, int screenH);

private:
    void addQuads(float x, float y, float scale, const glm::vec4& color, const std::string& text);

    unsigned int prog_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int tex_ = 0;
    int locScreen_ = -1;
    int locTex_ = -1;
    std::vector<float> verts_; // x, y, u, v, r, g, b, a
};
