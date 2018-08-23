#include "gui.h"
#include "graphics.h"
#include "egl.h"
#include "java_application.h"
#include "imgui/imgui.h"
#include "utils/log.h"
#include "gldebug.h"
#include "shader.h"
#include "texture.h"
#include <GLES2/gl2.h>
#include <vector>
#include <algorithm>
#include <utils/make_unique.h>

namespace {
	ImGuiIO* g_Io = nullptr;
    bool g_WindowOpenState = true;
    float g_MousePos[2] = {0.0f, 0.0f};
    bool g_MousePress[10] = {false};

    void HandleInput(int action, float x, float y)
    {
        switch(action)
        {
            case AMOTION_EVENT_ACTION_DOWN:
                g_MousePress[0] = true;
                break;
            case AMOTION_EVENT_ACTION_UP:
                g_MousePress[0] = false;
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                break;
        }
        g_MousePos[0] = x;
        g_MousePos[1] = y;

        //inject input
        if (g_Io)
        {
            g_Io->MouseDown[0] = g_MousePress[0];
            g_Io->MousePos.x = g_MousePos[0];
            g_Io->MousePos.y = g_MousePos[1];
        }
    }

}

namespace Gui
{
    struct ButtonImpl;
    std::vector<std::unique_ptr<ButtonImpl> > g_Buttons;

    struct ButtonImpl
    {
        friend void Gui::StartDraw();
        friend Gui::ButtonImpl* Gui::CreateButton(const std::string& label, const std::function<void(Gui::ButtonImpl*)>& onClick);

    public:
        ButtonImpl(const std::string& label, const std::function<void(ButtonImpl*)>& onClick)
                : m_Label(label)
                , m_OnClick(onClick)
        {}

    private:
        void Invoke()
        {
            m_OnClick(this);
        }
    public:
        void Destroy()
        {
            g_Buttons.erase(std::remove_if(g_Buttons.begin(), g_Buttons.end(), [this](std::unique_ptr<ButtonImpl>& button) -> bool {
                if (button.get() == this)
                {
                    return true;
                }

                return false;
            }), g_Buttons.end());
        }
    private:
        std::string m_Label;
        std::function<void(ButtonImpl*)> m_OnClick;
    };
}

bool Gui::Initialize()
{
	ImGui::CreateContext();
	g_Io = &ImGui::GetIO();
    g_Io->DisplaySize.x = EGL::GetWidth();
    g_Io->DisplaySize.y = EGL::GetHeight();
    g_Io->FontGlobalScale = 5.0f;
    ImGui::GetStyle().TouchExtraPadding.x = 32.0f;
    ImGui::GetStyle().TouchExtraPadding.y = 32.0f;

    //Gui: font texture
    unsigned char* pixels;
    int width, height;
    g_Io->Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    Texture* tex = new Texture();
    tex->LoadFromBuffer(TextureFormat::Raw, pixels, width, height);
    g_Io->Fonts->TexID = (void*)tex;

    App::OnInput(AMOTION_EVENT_ACTION_DOWN, HandleInput);
    App::OnInput(AMOTION_EVENT_ACTION_UP, HandleInput);
    App::OnInput(AMOTION_EVENT_ACTION_MOVE, HandleInput);

    return true;
}

void Gui::Destroy()
{
    if (g_Io)
    {
        Texture *tex = static_cast<Texture *>(g_Io->Fonts->TexID);
        delete tex;
        g_Io->Fonts->TexID = (void *) 0;
    }
}

void Gui::SetBufferSize(int width, int height)
{
    if (g_Io)
    {
        g_Io->DisplaySize.x = width;
        g_Io->DisplaySize.y = height;
    }
}

Gui::ButtonImpl* Gui::CreateButton(const std::string& label, const std::function<void(Gui::ButtonImpl*)>& onClick)
{
    g_Buttons.push_back(std::make_unique<Gui::ButtonImpl>(label, onClick));
    return g_Buttons.back().get();
}

void Gui::StartDraw()
{
    g_Io->DeltaTime = 1.0f/60.0f;

    // Call NewFrame(), after this point you can use ImGui::* functions anytime
    ImGui::NewFrame();

    //Demo
    //ImGui::ShowDemoWindow(&g_WindowOpenState);

    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowSize(ImVec2(EGL::GetWidth(), EGL::GetHeight()), ImGuiCond_Always);
    ImGui::Begin("root", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("Vulkan");
    std::for_each(g_Buttons.begin(), g_Buttons.end(), [](std::unique_ptr<ButtonImpl>& button){
        if (ImGui::Button(button->m_Label.c_str()))
        {
            LOGI("Invoking button \"%s\"", button->m_Label.c_str());
            button->Invoke();
        }
    });

    ImGui::End();
}

void Gui::EndDraw(Shader::Program* shader, unsigned int& vertexBuffer, unsigned int& indexBuffer)
{
    //GUI: Render frame here
    ImGui::Render();

    CHECK_GL(glEnable(GL_SCISSOR_TEST));

    Texture* tex = reinterpret_cast<Texture*>(g_Io->Fonts->TexID);
    tex->Bind(shader, 0);

    CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));

    CHECK_GL(glEnableVertexAttribArray(shader->GetAttribute("aPos")));
    CHECK_GL(glEnableVertexAttribArray(shader->GetAttribute("aTex")));
    CHECK_GL(glEnableVertexAttribArray(shader->GetAttribute("aColor")));
    CHECK_GL(glVertexAttribPointer(shader->GetAttribute("aPos"), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos)));
    CHECK_GL(glVertexAttribPointer(shader->GetAttribute("aTex"), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv)));
    CHECK_GL(glVertexAttribPointer(shader->GetAttribute("aColor"), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col)));

    ImDrawData* draw_data = ImGui::GetDrawData();
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_ptr = 0;

        CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
        CHECK_GL(glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW));

        CHECK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
        CHECK_GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                CHECK_GL(glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId));
                CHECK_GL(glScissor((int)pcmd->ClipRect.x, (int)(EGL::GetHeight() - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y)));
                CHECK_GL(glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer_ptr));
            }
            idx_buffer_ptr += pcmd->ElemCount;
        }
    }
    CHECK_GL(glDisable(GL_SCISSOR_TEST));
}