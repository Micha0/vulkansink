#pragma once
#include <string>
#include <functional>

namespace Shader {
	class Program;
}

namespace Gui {
	struct ButtonImpl;

	bool Initialize();
	void Destroy();
	void SetBufferSize(int width, int height);
	ButtonImpl* CreateButton(const std::string& label, const std::function<void(ButtonImpl*)>& onClick);
	void StartDraw();
	void EndDraw(Shader::Program* shader, unsigned int& vertextBuffer, unsigned int& indexBuffer);
}