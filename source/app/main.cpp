/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <initializer_list>
#include <jni.h>
#include <errno.h>
#include <cassert>
#include <cstdlib>
#include <cstring>

//#include <GLES/gl.h>
//#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
//#include <GLES3/gl3.h>
//#include <GLES3/gl3ext.h>

#include <android/sensor.h>
#include <android_native_app_glue.h>

#include "profiler/profiler.h"

#include "utils/log.h"
#include "utils/opengl.h"
#include "imgui/imgui.h"

#include "graphics/egl.h"
#include "graphics/shader.h"
#include "graphics/gldebug.h"


/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
    int pressed;
};

struct window_state {
    bool p_open = true;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;

    ImGuiIO* imguiio;
    Shader::Program* shader;
    GLuint vertexBuffer;
    GLuint indexBuffer;

    struct window_state winstate;
};

static unsigned int engine_load_texture_raw(unsigned char* buffer, int width, int height)
{
// Create one OpenGL texture
    GLuint textureID;
    CHECK_GL(glGenTextures(1, &textureID));

// "Bind" the newly created texture : all future texture functions will modify this texture
    CHECK_GL(glBindTexture(GL_TEXTURE_2D, textureID));

// Give the image to OpenGL
    CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer));

    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));

    return textureID;
}

static void engine_matrix_ortho(int shaderLocation, float width, float height)
{
    static const float ortho_projection[4][4] =
        {
            { 2.0f/width, 0.0f,         0.0f, 0.0f },
            { 0.0f,       2.0f/-height, 0.0f, 0.0f },
            { 0.0f,       0.0f,        -1.0f, 0.0f },
            {-1.0f,       1.0f,         0.0f, 1.0f },
        };
    CHECK_GL(glUniformMatrix4fv(shaderLocation, 1, GL_FALSE, &ortho_projection[0][0]));
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine)
{
    if (!EGL::Initialize())
    {
        return -1;
    }

    engine->display = EGL::GetDisplay();
    engine->context = EGL::GetContext();
    engine->surface = EGL::GetSurface();
    engine->width = EGL::GetWidth();
    engine->height = EGL::GetHeight();
    engine->state.angle = 0;
    engine->state.pressed = 0;
    engine->winstate.p_open = true;

    //Shader: init
    engine->shader = new Shader::Program(
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

    //Gui: init
    ImGui::CreateContext();
    engine->imguiio = &ImGui::GetIO();
    engine->imguiio->DisplaySize.x = w;
    engine->imguiio->DisplaySize.y = h;
    engine->imguiio->FontGlobalScale = 4.0f;
    ImGui::GetStyle().TouchExtraPadding.x = 32.0f;
    ImGui::GetStyle().TouchExtraPadding.y = 32.0f;

    //Gui: font texture
    unsigned char* pixels;
    int width, height;
    engine->imguiio->Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    engine->imguiio->Fonts->TexID = (void*)engine_load_texture_raw(pixels, width, height);

    //Buffers
    CHECK_GL(glGenBuffers(1, &engine->vertexBuffer));
    CHECK_GL(glGenBuffers(1, &engine->indexBuffer));


	Profiler::Initialize();

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
//    CHECK_GL(glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST));
    CHECK_GL(glDisable(GL_CULL_FACE));
//    CHECK_GL(glShadeModel(GL_SMOOTH));
    CHECK_GL(glDisable(GL_DEPTH_TEST));
    CHECK_GL(glEnable(GL_BLEND));
    CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    engine->animating = 1;
    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
	PROFILE;

    if (engine->display == NULL) {
        // No display.
        return;
    }

    // Just fill the screen with a color.
//	glClearColor(((float)engine->state.x)/engine->width, engine->state.angle,
//				 ((float)engine->state.y)/engine->height, 1);
    CHECK_GL(glViewport(0, 0, engine->width, engine->height));
    CHECK_GL(glClear(GL_COLOR_BUFFER_BIT));
    CHECK_GL(glClearColor(1.0, 0.0, 1.0, 1));

    //GUI: Frame start here
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f/60.0f;
//    io.MousePos.x = engine->state.x;
//    io.MousePos.y = engine->state.y;
//    io.MouseDown[0] = engine->state.pressed;
//    io.MouseDown[1] = false;

    // Call NewFrame(), after this point you can use ImGui::* functions anytime
    ImGui::NewFrame();

    //Test
    ImGui::SetNextWindowSize(ImVec2(550,680), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("ImGui Demo", &engine->winstate.p_open, ImGuiWindowFlags_MenuBar & ~ImGuiWindowFlags_Modal))
    {
        ImGui::Text("dear imgui says hello. (%s)", IMGUI_VERSION);
    }
    ImGui::End();

    ImGui::Text("Another text line (outside window?)", IMGUI_VERSION);


    //GUI: Render frame here
    ImGui::Render();
    //GUI: Commit Render
    CHECK_GL(glEnable(GL_SCISSOR_TEST));
    engine->shader->Use();

    engine_matrix_ortho(engine->shader->GetUniform("uProjMatrix"), engine->width, engine->height);

    CHECK_GL(glUniform1i(engine->shader->GetUniform("uTexture"), 0));
    //glBindTexture(0, (GLint)io.Fonts->TexID);

    CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, engine->vertexBuffer));
    CHECK_GL(glEnableVertexAttribArray(engine->shader->GetAttribute("aPos")));
    CHECK_GL(glEnableVertexAttribArray(engine->shader->GetAttribute("aTex")));
    CHECK_GL(glEnableVertexAttribArray(engine->shader->GetAttribute("aColor")));
    CHECK_GL(glVertexAttribPointer(engine->shader->GetAttribute("aPos"), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos)));
    CHECK_GL(glVertexAttribPointer(engine->shader->GetAttribute("aTex"), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv)));
    CHECK_GL(glVertexAttribPointer(engine->shader->GetAttribute("aColor"), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col)));

    ImDrawData* draw_data = ImGui::GetDrawData();
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_ptr = 0;

        CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, engine->vertexBuffer));
        CHECK_GL(glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW));

        CHECK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, engine->indexBuffer));
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
                CHECK_GL(glScissor((int)pcmd->ClipRect.x, (int)(engine->height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y)));
                CHECK_GL(glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer_ptr));
            }
            idx_buffer_ptr += pcmd->ElemCount;
        }
    }
    CHECK_GL(glDisable(GL_SCISSOR_TEST));

    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    ImGui::DestroyContext();

    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
    if (engine->vertexBuffer)
    {
        glDeleteBuffers(1, &engine->vertexBuffer);
        engine->vertexBuffer = 0;
    }
    if (engine->indexBuffer)
    {
        glDeleteBuffers(1, &engine->indexBuffer);
        engine->indexBuffer = 0;
    }
    if (engine->shader)
    {
        delete engine->shader;
    }
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
    {
        int source = AInputEvent_getSource(event);
        if (source == AINPUT_SOURCE_TOUCHSCREEN)
        {
            ImGuiIO& io = ImGui::GetIO();
            int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            switch(action){
                case AMOTION_EVENT_ACTION_DOWN:
                    engine->state.pressed = 1;
                    io.MouseDown[0] = true;
                    io.MousePos.x = engine->state.x = AMotionEvent_getX(event, 0);
                    io.MousePos.y = engine->state.y = AMotionEvent_getY(event, 0);
                    LOGI("State pressed");
                    break;
                case AMOTION_EVENT_ACTION_UP:
                    engine->state.pressed = 0;
                    io.MouseDown[0] = false;
                    io.MousePos.x = -FLT_MAX;
                    io.MousePos.y = -FLT_MAX;
                    LOGI("State released");
                    break;
                case AMOTION_EVENT_ACTION_MOVE:
                    io.MousePos.x = engine->state.x = AMotionEvent_getX(event, 0);
                    io.MousePos.y = engine->state.y = AMotionEvent_getY(event, 0);
                    LOGI("State moved");
                    break;
            }

        }
    }
    else //if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
    {
        return 0;
    }
    return 1;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor,
                                               (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;

    // Make sure glue isn't stripped.
    //app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(
                                        engine.sensorManager,
                                        ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(
                                    engine.sensorManager,
                                    state->looper, LOOPER_ID_USER,
                                    NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.

    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                                      (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                       &event, 1) > 0) {
//                        LOGI("accelerometer: x=%f y=%f z=%f",
//                             event.acceleration.x, event.acceleration.y,
//                             event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        engine_draw_frame(&engine);
    }
}
//END_INCLUDE(all)