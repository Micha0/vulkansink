#include <GLES2/gl2.h>
#include <map>
#include "gldebug.h"
#include "utils/log.h"

namespace {
    static const std::map<GLenum, std::string> g_GlErrorToString = {
        {GL_INVALID_ENUM, "GL_INVALID_ENUM: enum argument out of range"},
        {GL_INVALID_VALUE, "GL_INVALID_VALUE: Numeric argument out of range"},
        {GL_INVALID_OPERATION, "GL_INVALID_OPERATION: Operation illegal in current state"},
        {GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION: Framebuffer is incomplete"},
        {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY: Not enough memory left to execute command"}
    };
}

void GlDebug::CheckCall(const char* file, unsigned int line, const char* func, const char* call)
{
    GLenum error = glGetError();

    while (error != GL_NO_ERROR)
    {
        if (g_GlErrorToString.find(error) == g_GlErrorToString.end())
        {
            LOGE("%s[%u]: %s: OpenGL Error: %s 0x%04x\n", file, line, func, call, (int)error);
        }
        else
        {
            LOGE("%s[%u]: %s: OpenGL Error: %s 0x%04x %s\n", file, line, func, call, (int)error, g_GlErrorToString.at(error).c_str());
        }

        error = glGetError();
    }
}