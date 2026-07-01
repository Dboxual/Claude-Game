#include "debug_text.h"

#include "gl_loader.h"
#include "renderer.h" // compileProgram

#include <cctype>
#include <cstring>

namespace {

// Atlas layout: ASCII 32..127, 16 columns of 6x8 cells (glyphs are 5x7 with
// a 1px gutter). Undefined characters render blank.
constexpr int kCellW = 6;
constexpr int kCellH = 8;
constexpr int kCols = 16;
constexpr int kRows = 6;
constexpr int kAtlasW = kCols * kCellW; // 96
constexpr int kAtlasH = kRows * kCellH; // 48
constexpr int kFirstChar = 32;
constexpr int kLastChar = 127;

struct Glyph {
    char c;
    const char* rows[7];
};

const Glyph kGlyphs[] = {
    {'0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}},
    {'1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
    {'3', {" ### ", "#   #", "    #", "  ## ", "    #", "#   #", " ### "}},
    {'4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}},
    {'5', {"#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "}},
    {'6', {"  ## ", " #   ", "#    ", "#### ", "#   #", "#   #", " ### "}},
    {'7', {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}},
    {'8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
    {'9', {" ### ", "#   #", "#   #", " ####", "    #", "   # ", " ##  "}},
    {'A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}},
    {'C', {" ### ", "#   #", "#    ", "#    ", "#    ", "#   #", " ### "}},
    {'D', {"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}},
    {'E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}},
    {'F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}},
    {'G', {" ### ", "#   #", "#    ", "# ###", "#   #", "#   #", " ### "}},
    {'H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'I', {" ### ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'J', {"  ###", "   # ", "   # ", "   # ", "   # ", "#  # ", " ##  "}},
    {'K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
    {'M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}},
    {'N', {"#   #", "##  #", "##  #", "# # #", "#  ##", "#  ##", "#   #"}},
    {'O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}},
    {'Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}},
    {'R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}},
    {'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
    {'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}},
    {'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"}},
    {'X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}},
    {'Y', {"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}},
    {'+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "}},
    {'-', {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},
    {'.', {"     ", "     ", "     ", "     ", "     ", " ##  ", " ##  "}},
    {',', {"     ", "     ", "     ", "     ", " ##  ", " ##  ", " #   "}},
    {':', {"     ", " ##  ", " ##  ", "     ", " ##  ", " ##  ", "     "}},
    {'/', {"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "}},
    {'(', {"   # ", "  #  ", " #   ", " #   ", " #   ", "  #  ", "   # "}},
    {')', {" #   ", "  #  ", "   # ", "   # ", "   # ", "  #  ", " #   "}},
    {'%', {"##  #", "##  #", "   # ", "  #  ", " #   ", "#  ##", "#  ##"}},
};

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

} // namespace

bool DebugText::init() {
    prog_ = compileProgram(kTextVS, kTextFS);
    if (!prog_) return false;
    locScreen_ = glGetUniformLocation(prog_, "uScreen");
    locTex_ = glGetUniformLocation(prog_, "uTex");

    // Rasterize the glyph table into the atlas.
    unsigned char pixels[kAtlasW * kAtlasH] = {};
    for (const Glyph& g : kGlyphs) {
        int index = g.c - kFirstChar;
        if (index < 0 || index >= kCols * kRows) continue;
        int cellX = (index % kCols) * kCellW;
        int cellY = (index / kCols) * kCellH;
        for (int row = 0; row < 7; ++row) {
            const char* line = g.rows[row];
            for (int col = 0; col < 5 && line[col] != '\0'; ++col) {
                if (line[col] == '#') {
                    pixels[(cellY + row) * kAtlasW + cellX + col] = 255;
                }
            }
        }
    }

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kAtlasW, kAtlasH, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
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

void DebugText::shutdown() {
    if (tex_) glDeleteTextures(1, &tex_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (prog_) glDeleteProgram(prog_);
    tex_ = vbo_ = vao_ = prog_ = 0;
}

void DebugText::begin() {
    verts_.clear();
}

float DebugText::textWidth(const std::string& text, float scale) const {
    if (text.empty()) return 0.0f;
    return float(text.size()) * kCellW * scale - scale; // minus trailing gutter
}

void DebugText::add(float x, float y, float scale, const glm::vec4& color, const std::string& text) {
    glm::vec4 shadow(0.0f, 0.0f, 0.0f, color.a * 0.8f);
    addQuads(x + scale, y + scale, scale, shadow, text);
    addQuads(x, y, scale, color, text);
}

void DebugText::addCentered(float centerX, float y, float scale, const glm::vec4& color, const std::string& text) {
    add(centerX - textWidth(text, scale) * 0.5f, y, scale, color, text);
}

void DebugText::addQuads(float x, float y, float scale, const glm::vec4& color, const std::string& text) {
    float penX = x;
    for (char raw : text) {
        char c = char(std::toupper(static_cast<unsigned char>(raw)));
        if (c == '\n') {
            penX = x;
            y += kCellH * scale;
            continue;
        }
        int index = c - kFirstChar;
        if (c >= kFirstChar && c <= kLastChar && c != ' ' && index < kCols * kRows) {
            float cellX = float((index % kCols) * kCellW);
            float cellY = float((index / kCols) * kCellH);
            float u0 = cellX / kAtlasW;
            float v0 = cellY / kAtlasH;
            float u1 = (cellX + 5.0f) / kAtlasW;
            float v1 = (cellY + 7.0f) / kAtlasH;
            float x0 = penX, y0 = y;
            float x1 = penX + 5.0f * scale, y1 = y + 7.0f * scale;
            const float quad[6][8] = {
                {x0, y0, u0, v0, color.r, color.g, color.b, color.a},
                {x1, y0, u1, v0, color.r, color.g, color.b, color.a},
                {x1, y1, u1, v1, color.r, color.g, color.b, color.a},
                {x0, y0, u0, v0, color.r, color.g, color.b, color.a},
                {x1, y1, u1, v1, color.r, color.g, color.b, color.a},
                {x0, y1, u0, v1, color.r, color.g, color.b, color.a},
            };
            verts_.insert(verts_.end(), &quad[0][0], &quad[0][0] + 48);
        }
        penX += kCellW * scale;
    }
}

void DebugText::flush(int screenW, int screenH) {
    if (verts_.empty()) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(prog_);
    glUniform2f(locScreen_, float(screenW), float(screenH));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glUniform1i(locTex_, 0);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(verts_.size() * sizeof(float)), verts_.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(verts_.size() / 8));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}
