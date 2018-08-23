#pragma once

#include <EGL/egl.h>

namespace EGL
{
	EGLint GetWidth();
	EGLint GetHeight();
	EGLint GetFormat();
	EGLConfig GetConfig();
	EGLSurface GetSurface();
	EGLContext GetContext();
	bool Initialize(ANativeWindow* window);
	void Destroy();
	void Swap();
}