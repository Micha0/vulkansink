#pragma once

#include <GLES2/gl2.h>
#include <string>
#include <memory>
#include <map>

namespace Shader
{

	enum class ShaderType : GLenum
	{
		Vertex = GL_VERTEX_SHADER,
		Fragment = GL_FRAGMENT_SHADER,
	};

	class Program;

	class Shader
	{
	public:
		friend class Program;

		Shader(const char* source, ShaderType type);
		~Shader();
		bool Ok();

	private:
		GLuint GetObject();

		std::string m_ShaderSource;
		GLuint m_ShaderObject;
		bool m_IsOk;
	};

	class Program
	{
	public:
		Program(const char* vertexShaderSource, const char* fragmentShaderSource);
		Program(std::shared_ptr<Shader> vertexShader, std::shared_ptr<Shader> fragmentShader);
		~Program();
		bool Ok();
		GLuint GetObject();
		GLint GetAttribute(std::string attributeName);
		GLint GetUniform(std::string uniformName);
		void Use();

	private:

		std::shared_ptr<Shader> m_VertexShader;
		std::shared_ptr<Shader> m_FragmentShader;
		GLuint m_ProgramObject;

		std::map<std::string, GLint> m_CachedAttributeLocations;
		std::map<std::string, GLint> m_CachedUniformLocations;
		bool m_IsOk;
	};
}