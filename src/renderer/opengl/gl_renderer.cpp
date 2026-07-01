#include "renderer/opengl/gl_renderer.h"

#include "engine/log.h"
#include "platform/window.h"
#include "renderer/common/font5x7.h"

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

const char* kTextVS = R"GLSL(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
uniform vec2 uScreen;
out vec2 vUV;
out vec4 vColor;
void main() {
    vec2 ndc = vec2(aPos.x / uScreen.x * 2.0 - 1.0, 1.0 - aPos.y / uScreen.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)GLSL";

const char* kTextFS = R"GLSL(
#version 330 core
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uTex;
out vec4 fragColor;
void main() {
    float a = texture(uTex, vUV).r;
    fragColor = vec4(vColor.rgb, vColor.a * a);
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
        eng::logError("Shader compile error (%s):\n%.*s",
                      type == GL_VERTEX_SHADER ? "vertex" : "fragment", int(len), log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

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
        eng::logError("Program link error:\n%.*s", int(len), log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

} // namespace

bool GLRenderer::init(IWindow& window) {
    window_ = &window;

    if (!loadGLFunctions(window.glProcLoader())) {
        eng::logError("Failed to load OpenGL 3.3 entry points");
        return false;
    }
    eng::logInfo("GL renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    eng::logInfo("GL version:  %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    // World pipeline.
    worldProg_ = compileProgram(kWorldVS, kWorldFS);
    if (!worldProg_) return false;
    locProj_ = glGetUniformLocation(worldProg_, "uProj");
    locView_ = glGetUniformLocation(worldProg_, "uView");
    locModel_ = glGetUniformLocation(worldProg_, "uModel");
    locColor_ = glGetUniformLocation(worldProg_, "uColor");
    locChecker_ = glGetUniformLocation(worldProg_, "uChecker");

    glGenVertexArrays(1, &cubeVao_);
    glGenBuffers(1, &cubeVbo_);
    glBindVertexArray(cubeVao_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Text pipeline.
    textProg_ = compileProgram(kTextVS, kTextFS);
    if (!textProg_) return false;
    locScreen_ = glGetUniformLocation(textProg_, "uScreen");
    locTex_ = glGetUniformLocation(textProg_, "uTex");

    std::vector<std::uint8_t> atlas = font5x7::buildAtlas();
    glGenTextures(1, &fontTex_);
    glBindTexture(GL_TEXTURE_2D, fontTex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, font5x7::kAtlasW, font5x7::kAtlasH, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenVertexArrays(1, &textVao_);
    glGenBuffers(1, &textVbo_);
    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return true;
}

void GLRenderer::shutdown() {
    if (fontTex_) glDeleteTextures(1, &fontTex_);
    if (textVbo_) glDeleteBuffers(1, &textVbo_);
    if (textVao_) glDeleteVertexArrays(1, &textVao_);
    if (textProg_) glDeleteProgram(textProg_);
    if (cubeVbo_) glDeleteBuffers(1, &cubeVbo_);
    if (cubeVao_) glDeleteVertexArrays(1, &cubeVao_);
    if (worldProg_) glDeleteProgram(worldProg_);
    fontTex_ = textVbo_ = textVao_ = textProg_ = 0;
    cubeVbo_ = cubeVao_ = worldProg_ = 0;
}

void GLRenderer::render(const RenderFrame& frame) {
    glViewport(0, 0, frame.viewportW, frame.viewportH);
    glClearColor(0.55f, 0.65f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawBoxes(frame);
    drawTexts(frame);

    window_->present();
}

void GLRenderer::drawBoxes(const RenderFrame& frame) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glUseProgram(worldProg_);
    glUniformMatrix4fv(locProj_, 1, GL_FALSE, glm::value_ptr(frame.proj));
    glUniformMatrix4fv(locView_, 1, GL_FALSE, glm::value_ptr(frame.view));

    glBindVertexArray(cubeVao_);
    for (const BoxDraw& box : frame.boxes) {
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), box.center), box.size);
        glUniformMatrix4fv(locModel_, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(locColor_, box.color.r, box.color.g, box.color.b);
        glUniform1i(locChecker_, box.checkerTop ? 1 : 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);
}

void GLRenderer::drawTexts(const RenderFrame& frame) {
    textVerts_.clear();
    for (const TextDraw& t : frame.texts) {
        float x = t.centered ? t.x - font5x7::textWidth(t.text, t.scale) * 0.5f : t.x;
        const float shadow[4] = {0.0f, 0.0f, 0.0f, t.color.a * 0.8f};
        const float rgba[4] = {t.color.r, t.color.g, t.color.b, t.color.a};
        addTextQuads(x + t.scale, t.y + t.scale, t.scale, shadow, t.text);
        addTextQuads(x, t.y, t.scale, rgba, t.text);
    }
    if (textVerts_.empty()) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(textProg_);
    glUniform2f(locScreen_, float(frame.viewportW), float(frame.viewportH));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex_);
    glUniform1i(locTex_, 0);

    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(textVerts_.size() * sizeof(float)),
                 textVerts_.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(textVerts_.size() / 8));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void GLRenderer::addTextQuads(float x, float y, float scale, const float rgba[4], const std::string& text) {
    using namespace font5x7;
    float penX = x;
    for (char raw : text) {
        if (raw == '\n') {
            penX = x;
            y += kCellH * scale;
            continue;
        }
        int index = cellIndex(raw);
        if (index >= 0) {
            float cellX = float((index % kCols) * kCellW);
            float cellY = float((index / kCols) * kCellH);
            float u0 = cellX / kAtlasW;
            float v0 = cellY / kAtlasH;
            float u1 = (cellX + float(kGlyphW)) / kAtlasW;
            float v1 = (cellY + float(kGlyphH)) / kAtlasH;
            float x0 = penX, y0 = y;
            float x1 = penX + kGlyphW * scale, y1 = y + kGlyphH * scale;
            const float quad[6][8] = {
                {x0, y0, u0, v0, rgba[0], rgba[1], rgba[2], rgba[3]},
                {x1, y0, u1, v0, rgba[0], rgba[1], rgba[2], rgba[3]},
                {x1, y1, u1, v1, rgba[0], rgba[1], rgba[2], rgba[3]},
                {x0, y0, u0, v0, rgba[0], rgba[1], rgba[2], rgba[3]},
                {x1, y1, u1, v1, rgba[0], rgba[1], rgba[2], rgba[3]},
                {x0, y1, u0, v1, rgba[0], rgba[1], rgba[2], rgba[3]},
            };
            textVerts_.insert(textVerts_.end(), &quad[0][0], &quad[0][0] + 48);
        }
        penX += kCellW * scale;
    }
}
