#pragma once

#include "gl_loader.h"
#include "world.h"

#include <glm/glm.hpp>

// Compiles and links a GLSL program; returns 0 and logs the info log on error.
GLuint compileProgram(const char* vsSrc, const char* fsSrc);

class Renderer {
public:
    bool init();
    void shutdown();
    void beginFrame(int pixelW, int pixelH, const glm::mat4& proj, const glm::mat4& view);
    void drawWorld(const World& world);

private:
    void drawBox(const WorldBox& box);

    GLuint prog_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLint locProj_ = -1;
    GLint locView_ = -1;
    GLint locModel_ = -1;
    GLint locColor_ = -1;
    GLint locChecker_ = -1;
};
