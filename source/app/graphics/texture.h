#pragma once

namespace Shader
{
	class Program;
}

enum class TextureFormat : unsigned
{
	Raw = 0,

	Count
};

class Texture
{
public:
	Texture();

	void LoadFromBuffer(TextureFormat format, unsigned char* data, unsigned int width, unsigned int height);
	void Bind(Shader::Program* shader, unsigned int textureUnit);
private:
	unsigned int m_TextureId;
	bool m_Loaded;
};
