#pragma once

// Minimal OpenGL 3.3 core loader. Entry points are resolved through a proc
// loader supplied by the platform window (SDL_GL_GetProcAddress on desktop),
// so this file has no platform dependency and no external loader library.

#include <cstddef>

#if defined(_WIN32)
    #define GLAPIENTRY __stdcall
#else
    #define GLAPIENTRY
#endif

using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLbyte = signed char;
using GLubyte = unsigned char;
using GLshort = short;
using GLushort = unsigned short;
using GLint = int;
using GLuint = unsigned int;
using GLsizei = int;
using GLfloat = float;
using GLclampf = float;
using GLdouble = double;
using GLchar = char;
using GLsizeiptr = std::ptrdiff_t;
using GLintptr = std::ptrdiff_t;

#define GL_FALSE                 0
#define GL_TRUE                  1
#define GL_DEPTH_BUFFER_BIT      0x00000100
#define GL_COLOR_BUFFER_BIT      0x00004000
#define GL_TRIANGLES             0x0004
#define GL_LEQUAL                0x0203
#define GL_SRC_ALPHA             0x0302
#define GL_ONE_MINUS_SRC_ALPHA   0x0303
#define GL_BACK                  0x0405
#define GL_CCW                   0x0901
#define GL_CULL_FACE             0x0B44
#define GL_DEPTH_TEST            0x0B71
#define GL_BLEND                 0x0BE2
#define GL_UNPACK_ALIGNMENT      0x0CF5
#define GL_TEXTURE_2D            0x0DE1
#define GL_UNSIGNED_BYTE         0x1401
#define GL_UNSIGNED_INT          0x1405
#define GL_FLOAT                 0x1406
#define GL_RED                   0x1903
#define GL_VENDOR                0x1F00
#define GL_RENDERER              0x1F01
#define GL_VERSION               0x1F02
#define GL_NEAREST               0x2600
#define GL_LINEAR                0x2601
#define GL_TEXTURE_MAG_FILTER    0x2800
#define GL_TEXTURE_MIN_FILTER    0x2801
#define GL_TEXTURE_WRAP_S        0x2802
#define GL_TEXTURE_WRAP_T        0x2803
#define GL_CLAMP_TO_EDGE         0x812F
#define GL_R8                    0x8229
#define GL_TEXTURE0              0x84C0
#define GL_ARRAY_BUFFER          0x8892
#define GL_STATIC_DRAW           0x88E4
#define GL_DYNAMIC_DRAW          0x88E8
#define GL_FRAGMENT_SHADER       0x8B30
#define GL_VERTEX_SHADER         0x8B31
#define GL_COMPILE_STATUS        0x8B81
#define GL_LINK_STATUS           0x8B82

#define GL_FUNCTIONS(X) \
    X(const GLubyte*, glGetString, GLenum) \
    X(GLenum, glGetError, void) \
    X(void, glEnable, GLenum) \
    X(void, glDisable, GLenum) \
    X(void, glClear, GLbitfield) \
    X(void, glClearColor, GLfloat, GLfloat, GLfloat, GLfloat) \
    X(void, glViewport, GLint, GLint, GLsizei, GLsizei) \
    X(void, glDepthFunc, GLenum) \
    X(void, glBlendFunc, GLenum, GLenum) \
    X(void, glCullFace, GLenum) \
    X(void, glFrontFace, GLenum) \
    X(void, glPixelStorei, GLenum, GLint) \
    X(void, glGenTextures, GLsizei, GLuint*) \
    X(void, glDeleteTextures, GLsizei, const GLuint*) \
    X(void, glBindTexture, GLenum, GLuint) \
    X(void, glTexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) \
    X(void, glTexParameteri, GLenum, GLenum, GLint) \
    X(void, glActiveTexture, GLenum) \
    X(void, glDrawArrays, GLenum, GLint, GLsizei) \
    X(GLuint, glCreateShader, GLenum) \
    X(void, glShaderSource, GLuint, GLsizei, const GLchar* const*, const GLint*) \
    X(void, glCompileShader, GLuint) \
    X(void, glGetShaderiv, GLuint, GLenum, GLint*) \
    X(void, glGetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*) \
    X(void, glDeleteShader, GLuint) \
    X(GLuint, glCreateProgram, void) \
    X(void, glAttachShader, GLuint, GLuint) \
    X(void, glLinkProgram, GLuint) \
    X(void, glGetProgramiv, GLuint, GLenum, GLint*) \
    X(void, glGetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*) \
    X(void, glUseProgram, GLuint) \
    X(void, glDeleteProgram, GLuint) \
    X(GLint, glGetUniformLocation, GLuint, const GLchar*) \
    X(void, glUniform1i, GLint, GLint) \
    X(void, glUniform1f, GLint, GLfloat) \
    X(void, glUniform2f, GLint, GLfloat, GLfloat) \
    X(void, glUniform3f, GLint, GLfloat, GLfloat, GLfloat) \
    X(void, glUniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat) \
    X(void, glUniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*) \
    X(void, glGenVertexArrays, GLsizei, GLuint*) \
    X(void, glDeleteVertexArrays, GLsizei, const GLuint*) \
    X(void, glBindVertexArray, GLuint) \
    X(void, glGenBuffers, GLsizei, GLuint*) \
    X(void, glDeleteBuffers, GLsizei, const GLuint*) \
    X(void, glBindBuffer, GLenum, GLuint) \
    X(void, glBufferData, GLenum, GLsizeiptr, const void*, GLenum) \
    X(void, glBufferSubData, GLenum, GLintptr, GLsizeiptr, const void*) \
    X(void, glVertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) \
    X(void, glEnableVertexAttribArray, GLuint)

#define GL_DECLARE_FN(ret, name, ...) extern ret(GLAPIENTRY* name)(__VA_ARGS__);
GL_FUNCTIONS(GL_DECLARE_FN)
#undef GL_DECLARE_FN

// Matches IWindow::GLProc; kept as a plain typedef so this header stays
// dependency-free.
using GLProcLoader = void* (*)(const char* name);

// Resolves every entry point above. Requires a current GL context. Returns
// false if any function is missing.
bool loadGLFunctions(GLProcLoader getProc);
