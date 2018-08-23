#include "graphics.h"
#include "shader.h"
#include "egl.h"
#include "gui.h"
#include "gldebug.h"
#include "utils/make_unique.h"
#include "utils/log.h"
#include "utils/opengl.h"
#include <map>
#include <string>
#include <algorithm>
#include <GLES2/gl2.h>

namespace {
	std::map<std::string, std::unique_ptr<Shader::Program> > g_Shaders;
	float g_OrthoProjection[16];
	GLuint g_VertexBuffer = 0;
	GLuint g_IndexBuffer = 0;
}

bool Graphics::Initialize(ANativeWindow* window)
{
	if (!EGL::Initialize(window))
	{
		LOGE("Unable to initialize EGL");
		return false;
	}

	Graphics::SetBufferSize(EGL::GetWidth(), EGL::GetHeight());

	//create buffers
    CHECK_GL(glGenBuffers(1, &g_VertexBuffer));
    CHECK_GL(glGenBuffers(1, &g_IndexBuffer));

	//create default shader
	g_Shaders["default"] = std::make_unique<Shader::Program>(
        "#version 100\n"
        "uniform mat4 uProjMatrix;\n"
        "attribute vec2 aPos;\n"
        "attribute vec2 aTex;\n"
        "attribute vec4 aColor;\n"
        "varying vec2 vTex;\n"
        "varying vec4 vColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = uProjMatrix * vec4(aPos, 0, 1);\n"
        "   vColor = aColor;\n"
        "   vTex = aTex;\n"
        "   \n"
        "}\n"
    ,
        "#version 100\n"
        "varying highp vec2 vTex;\n"
        "varying highp vec4 vColor;\n"
        "uniform sampler2D uTexture;\n"
        "void main()\n"
        "{\n"
        "   gl_FragColor = vColor * texture2D(uTexture, vTex);\n"
        "}\n"
        "\n"
    );

    // Check openGL on the system
    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("OpenGL Info: %s", info);
    }

	REPORT_GL_MAX(GL_MAX_VERTEX_UNIFORM_VECTORS);
	REPORT_GL_MAX(GL_MAX_TEXTURE_SIZE);
	REPORT_GL_MAX(GL_MAX_FRAGMENT_UNIFORM_VECTORS);
	REPORT_GL_MAX(GL_MAX_VARYING_VECTORS);
	REPORT_GL_MAX(GL_MAX_VERTEX_ATTRIBS);

    // Initialize GL state.
    //CHECK_GL(glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST));
    //CHECK_GL(glShadeModel(GL_SMOOTH));
    CHECK_GL(glDisable(GL_CULL_FACE));
    CHECK_GL(glDisable(GL_DEPTH_TEST));
    CHECK_GL(glEnable(GL_BLEND));
    CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    if (!Gui::Initialize())
    {
        return false;
    }

    return true;
}

void Graphics::SetBufferSize(int width_, int height_)
{
	float width = width_;
	float height = height_;
	float newOrtho[16] = {
			2.0f/width, 0.0f,         0.0f, 0.0f,
			0.0f,       2.0f/-height, 0.0f, 0.0f,
			0.0f,       0.0f,        -1.0f, 0.0f,
			-1.0f,       1.0f,         0.0f, 1.0f,
	};
    std::copy(std::begin(newOrtho), std::end(newOrtho), std::begin(g_OrthoProjection));

    Gui::SetBufferSize(width_, height_);
}

Shader::Program* Graphics::GetShader(const char* name)
{
	if (g_Shaders.find(name) != g_Shaders.end())
	{
		return g_Shaders[name].get();
	}
	else
	{
		return g_Shaders["default"].get();
	}
}

namespace {

}

void Graphics::Draw()
{
	//begin
    CHECK_GL(glViewport(0, 0, EGL::GetWidth(), EGL::GetHeight()));
    CHECK_GL(glClear(GL_COLOR_BUFFER_BIT));
    CHECK_GL(glClearColor(1.0, 0.0, 1.0, 1));

    Gui::StartDraw();

	//draw gui
	Shader::Program* shader = Graphics::GetShader("default");
	shader->Use();

	CHECK_GL(glUniformMatrix4fv(shader->GetUniform("uProjMatrix"), 1, GL_FALSE, g_OrthoProjection));

	Gui::EndDraw(shader, g_VertexBuffer, g_IndexBuffer);

	//end
	EGL::Swap();
}

void Graphics::Destroy()
{
	Gui::Destroy();
	EGL::Destroy();
}