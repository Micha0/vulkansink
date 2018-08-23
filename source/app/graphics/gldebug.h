#pragma once

namespace GlDebug
{
    void CheckCall(const char* file, unsigned int line, const char* func, const char* call);
}

#ifndef NDEBUG
#define CHECK_GL(a) a; GlDebug::CheckCall(__FILE__, __LINE__, __FUNCTION__, #a)
#else
#define CHECK_GL(a) a;
#endif