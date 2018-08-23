#include "texture.h"
#include "gldebug.h"
#include "shader.h"
#include <GLES2/gl2.h>

Texture::Texture()
	: m_Loaded(false)
	, m_TextureId(0)
{
}

void Texture::LoadFromBuffer(TextureFormat format, unsigned char* data, unsigned int width, unsigned int height)
{
    CHECK_GL(glGenTextures(1, &m_TextureId));

    CHECK_GL(glBindTexture(GL_TEXTURE_2D, m_TextureId));

	if (format == TextureFormat::Raw)
	{
	    CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
	}

    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));

    CHECK_GL(glBindTexture(GL_TEXTURE_2D, 0));

    m_Loaded = true;
}

void Texture::Bind(Shader::Program* shader, unsigned int textureUnit)
{
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, m_TextureId));
	CHECK_GL(glActiveTexture(GL_TEXTURE1 + textureUnit));
    CHECK_GL(glUniform1i(shader->GetUniform("uTexture"), textureUnit));
}