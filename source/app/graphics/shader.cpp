#include "shader.h"
#include "utils/log.h"
#include "utils/make_unique.h"
#include <memory>

Shader::Shader::Shader(const char* source, ShaderType type)
    : m_ShaderSource(source)
    , m_IsOk(false)
    , m_ShaderObject(0xFFFFFFFF)
{
    GLint compiled;

    // Create the shader object
    m_ShaderObject = glCreateShader((GLenum)type);

    if (m_ShaderObject == 0)
    {
        return;
    }

    // Load the shader source
    const char* shaderSourceMem = m_ShaderSource.c_str();
    glShaderSource(m_ShaderObject, 1, &shaderSourceMem, NULL);

    // Compile the shader
    glCompileShader(m_ShaderObject);
    // Check the compile status
    glGetShaderiv(m_ShaderObject, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(m_ShaderObject, GL_INFO_LOG_LENGTH, &infoLen);

        if(infoLen > 1)
        {
            std::unique_ptr<char[]> infoLog = std::make_unique<char[]>(infoLen);
            glGetShaderInfoLog(m_ShaderObject, infoLen, NULL, infoLog.get());
            LOGE("Error compiling shader:\n%s\n", infoLog.get());
            LOGI("Shader Source:\n%s\n", m_ShaderSource.c_str());
        }
        glDeleteShader(m_ShaderObject);
        m_ShaderObject = 0xFFFFFFFF;
        return;
    }

    m_IsOk = true;
}

Shader::Shader::~Shader()
{
    m_IsOk = false;
}

GLuint Shader::Shader::GetObject()
{
    return m_ShaderObject;
}

bool Shader::Shader::Ok()
{
    return m_IsOk;
}

Shader::Program::Program(const char* vertexShaderSource, const char* fragmentShaderSource)
    : Program(
        std::make_shared<Shader>(vertexShaderSource, ShaderType::Vertex),
        std::make_shared<Shader>(fragmentShaderSource, ShaderType::Fragment)
    )
{
}

Shader::Program::Program(std::shared_ptr<Shader> vertexShader, std::shared_ptr<Shader> fragmentShader)
    : m_VertexShader(vertexShader)
    , m_FragmentShader(fragmentShader)
    , m_IsOk(false)
{
    if (!m_VertexShader || !m_VertexShader->Ok())
    {
        LOGE("Error: Unable to create program: VertexShader not OK!");
        return;
    }
    if (!m_FragmentShader || !m_FragmentShader->Ok())
    {
        LOGE("Error: Unable to create program: FragmentShader not OK!");
        return;
    }

    // Create the program object
    m_ProgramObject = glCreateProgram();
    if (m_ProgramObject == 0)
    {
        return;
    }

    glAttachShader(m_ProgramObject, m_VertexShader->GetObject());
    glAttachShader(m_ProgramObject, m_FragmentShader->GetObject());

    // Bind vPosition to attribute 0
    //glBindAttribLocation(programObject, 0, "vPosition");

    // Link the program
    glLinkProgram(m_ProgramObject);

    // Check the link status
    GLint linked;
    glGetProgramiv(m_ProgramObject, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(m_ProgramObject, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1)
        {
            std::unique_ptr<char[]> infoLog = std::make_unique<char[]>(infoLen);
            glGetProgramInfoLog(m_ProgramObject, infoLen, NULL, infoLog.get());
            LOGE("Error linking program:\n%s\n", infoLog.get());
        }
        glDeleteProgram(m_ProgramObject);
        return;
    }

    m_IsOk = true;
}

Shader::Program::~Program()
{
    if (m_FragmentShader)
    {
        glDetachShader(m_ProgramObject, m_FragmentShader->GetObject());
        m_FragmentShader.reset();
    }
    if (m_VertexShader)
    {
        glDetachShader(m_ProgramObject, m_VertexShader->GetObject());
        m_VertexShader.reset();
    }
}

bool Shader::Program::Ok()
{
    if (m_VertexShader && m_VertexShader->Ok() && m_FragmentShader && m_FragmentShader->Ok())
    {
        return m_IsOk;
    }
    return false;
}

GLuint Shader::Program::GetObject()
{
    if (!Ok())
    {
        return 0;
    }
    return m_ProgramObject;
}

GLint Shader::Program::GetAttribute(std::string attributeName)
{
    if (!Ok())
    {
        return -1;
    }
    //TODO: add check whether the attribute really exists..
    if (m_CachedAttributeLocations.find(attributeName) == m_CachedAttributeLocations.end())
    {
        m_CachedAttributeLocations[attributeName] = glGetAttribLocation(m_ProgramObject, attributeName.c_str());
    }
    return m_CachedAttributeLocations[attributeName];
}

GLint Shader::Program::GetUniform(std::string uniformName)
{
    if (!Ok())
    {
        return -1;
    }
    //TODO: add check whether the uniform really exists..
    if (m_CachedUniformLocations.find(uniformName) == m_CachedUniformLocations.end())
    {
        m_CachedUniformLocations[uniformName] = glGetUniformLocation(m_ProgramObject, uniformName.c_str());
    }
    return m_CachedUniformLocations[uniformName];
}

void Shader::Program::Use()
{
    if (!Ok())
    {
        return;
    }
    glUseProgram(m_ProgramObject);
}
