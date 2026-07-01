#include "renderer/opengl/gl_loader.h"

#include "engine/log.h"

#define GL_DEFINE_FN(ret, name, ...) ret(GLAPIENTRY* name)(__VA_ARGS__) = nullptr;
GL_FUNCTIONS(GL_DEFINE_FN)
#undef GL_DEFINE_FN

bool loadGLFunctions(GLProcLoader getProc) {
    if (!getProc) {
        eng::logError("GL loader: no proc loader supplied by the window");
        return false;
    }
    int missing = 0;
#define GL_LOAD_FN(ret, name, ...)                                          \
    name = reinterpret_cast<ret(GLAPIENTRY*)(__VA_ARGS__)>(getProc(#name)); \
    if (!name) {                                                            \
        eng::logError("GL loader: missing entry point %s", #name);          \
        ++missing;                                                          \
    }
    GL_FUNCTIONS(GL_LOAD_FN)
#undef GL_LOAD_FN
    return missing == 0;
}
