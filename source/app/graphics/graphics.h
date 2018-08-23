#pragma once

#include <android/native_window.h>

namespace Shader
{
	class Program;
}

namespace Graphics
{
	bool Initialize(ANativeWindow* window);
	void SetBufferSize(int width, int height);
	Shader::Program* GetShader(const char* name);
	void Draw();
	void Destroy();
}