#pragma once

#include "client/renderer/opengl/gl_loader.h"
#include "client/renderer/renderer.h"

#include <vector>

// OpenGL 3.3 core backend for the desktop prototype (Windows/Linux/macOS).
class GLRenderer final : public IRenderer {
public:
    bool init(IWindow& window) override;
    void shutdown() override;
    void render(const RenderFrame& frame) override;
    const char* name() const override { return "OpenGL 3.3"; }

private:
    void drawBoxes(const RenderFrame& frame);
    void drawViewmodel(const RenderFrame& frame);
    void drawRects(const RenderFrame& frame);
    void drawTexts(const RenderFrame& frame);
    void addTextQuads(float x, float y, float scale, const float rgba[4], const std::string& text);

    IWindow* window_ = nullptr;

    GLuint worldProg_ = 0;
    GLuint cubeVao_ = 0;
    GLuint cubeVbo_ = 0;
    GLint locProj_ = -1, locView_ = -1, locModel_ = -1, locColor_ = -1, locChecker_ = -1;
    GLint locNormalMat_ = -1, locEmissive_ = -1;

    GLuint textProg_ = 0;
    GLuint textVao_ = 0;
    GLuint textVbo_ = 0;
    GLuint fontTex_ = 0;
    GLint locScreen_ = -1, locTex_ = -1;
    std::vector<float> textVerts_; // x, y, u, v, r, g, b, a

    GLuint rectProg_ = 0;
    GLuint rectVao_ = 0;
    GLuint rectVbo_ = 0;
    GLint locRectScreen_ = -1;
    std::vector<float> rectVerts_; // x, y, r, g, b, a
};
