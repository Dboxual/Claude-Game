#include "renderer.h"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {

const char* kWorldVS = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;
out vec3 vNormal;
out vec3 vWorldPos;
void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = mat3(uModel) * aNormal; // axis-aligned scale only, normalize in FS
    gl_Position = uProj * uView * wp;
}
)GLSL";

const char* kWorldFS = R"GLSL(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
uniform vec3 uColor;
uniform int uChecker;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(vec3(0.45, 1.0, 0.3))), 0.0);
    vec3 col = uColor * (0.35 + 0.65 * diff);
    if (uChecker == 1 && n.y > 0.9) {
        float c = mod(floor(vWorldPos.x) + floor(vWorldPos.z), 2.0);
        col *= (c < 0.5) ? 0.82 : 1.08;
    }
    float fog = clamp(length(vWorldPos) / 140.0, 0.0, 0.35);
    col = mix(col, vec3(0.55, 0.65, 0.75), fog);
    fragColor = vec4(col, 1.0);
}
)GLSL";

// Unit cube centered at the origin, CCW winding seen from outside.
// 36 vertices, interleaved position (3) + normal (3).
const float kCubeVerts[] = {
    // +Z
    -0.5f, -0.5f, 0.5f, 0, 0, 1,   0.5f, -0.5f, 0.5f, 0, 0, 1,   0.5f, 0.5f, 0.5f, 0, 0, 1,
    -0.5f, -0.5f, 0.5f, 0, 0, 1,   0.5f, 0.5f, 0.5f, 0, 0, 1,   -0.5f, 0.5f, 0.5f, 0, 0, 1,
    // -Z
    0.5f, -0.5f, -0.5f, 0, 0, -1,  -0.5f, -0.5f, -0.5f, 0, 0, -1, -0.5f, 0.5f, -0.5f, 0, 0, -1,
    0.5f, -0.5f, -0.5f, 0, 0, -1,  -0.5f, 0.5f, -0.5f, 0, 0, -1,  0.5f, 0.5f, -0.5f, 0, 0, -1,
    // +X
    0.5f, -0.5f, 0.5f, 1, 0, 0,    0.5f, -0.5f, -0.5f, 1, 0, 0,   0.5f, 0.5f, -0.5f, 1, 0, 0,
    0.5f, -0.5f, 0.5f, 1, 0, 0,    0.5f, 0.5f, -0.5f, 1, 0, 0,    0.5f, 0.5f, 0.5f, 1, 0, 0,
    // -X
    -0.5f, -0.5f, -0.5f, -1, 0, 0, -0.5f, -0.5f, 0.5f, -1, 0, 0,  -0.5f, 0.5f, 0.5f, -1, 0, 0,
    -0.5f, -0.5f, -0.5f, -1, 0, 0, -0.5f, 0.5f, 0.5f, -1, 0, 0,   -0.5f, 0.5f, -0.5f, -1, 0, 0,
    // +Y
    -0.5f, 0.5f, 0.5f, 0, 1, 0,    0.5f, 0.5f, 0.5f, 0, 1, 0,     0.5f, 0.5f, -0.5f, 0, 1, 0,
    -0.5f, 0.5f, 0.5f, 0, 1, 0,    0.5f, 0.5f, -0.5f, 0, 1, 0,    -0.5f, 0.5f, -0.5f, 0, 1, 0,
    // -Y
    -0.5f, -0.5f, -0.5f, 0, -1, 0, 0.5f, -0.5f, -0.5f, 0, -1, 0,  0.5f, -0.5f, 0.5f, 0, -1, 0,
    -0.5f, -0.5f, -0.5f, 0, -1, 0, 0.5f, -0.5f, 0.5f, 0, -1, 0,   -0.5f, -0.5f, 0.5f, 0, -1, 0,
};

GLuint compileStage(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        SDL_Log("Shader compile error (%s):\n%.*s",
                type == GL_VERTEX_SHADER ? "vertex" : "fragment", int(len), log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

GLuint compileProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileStage(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei len = 0;
        glGetProgramInfoLog(prog, sizeof(log), &len, log);
        SDL_Log("Program link error:\n%.*s", int(len), log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

bool Renderer::init() {
    prog_ = compileProgram(kWorldVS, kWorldFS);
    if (!prog_) return false;

    locProj_ = glGetUniformLocation(prog_, "uProj");
    locView_ = glGetUniformLocation(prog_, "uView");
    locModel_ = glGetUniformLocation(prog_, "uModel");
    locColor_ = glGetUniformLocation(prog_, "uColor");
    locChecker_ = glGetUniformLocation(prog_, "uChecker");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return true;
}

void Renderer::shutdown() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (prog_) glDeleteProgram(prog_);
    vbo_ = vao_ = prog_ = 0;
}

void Renderer::beginFrame(int pixelW, int pixelH, const glm::mat4& proj, const glm::mat4& view) {
    glViewport(0, 0, pixelW, pixelH);
    glClearColor(0.55f, 0.65f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glUseProgram(prog_);
    glUniformMatrix4fv(locProj_, 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(locView_, 1, GL_FALSE, glm::value_ptr(view));
}

void Renderer::drawWorld(const World& world) {
    glBindVertexArray(vao_);
    for (const WorldBox& wb : world.boxes()) {
        drawBox(wb);
    }
    glBindVertexArray(0);
}

void Renderer::drawBox(const WorldBox& wb) {
    glm::vec3 center = (wb.box.min + wb.box.max) * 0.5f;
    glm::vec3 size = wb.box.max - wb.box.min;
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), center), size);
    glUniformMatrix4fv(locModel_, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(locColor_, wb.color.r, wb.color.g, wb.color.b);
    glUniform1i(locChecker_, wb.checkerTop ? 1 : 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}
