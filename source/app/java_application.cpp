#include "java_application.h"
#include "utils/log.h"
#include "graphics/graphics.h"
#include "graphics/vulkan-test.h"
#include <unordered_map>
#include <algorithm>
#include <android/looper.h>
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/choreographer.h>
#include <graphics/gui.h>
#include <graphics/wsi.h>

namespace {
    typedef std::unordered_multimap<int32_t, App::CommandEventCallbackType > g_CommandEventHandlersType;
    typedef std::unordered_multimap<int32_t, App::InputEventCallbackType > g_InputEventHandlersType;
    g_CommandEventHandlersType g_CommandEventHandlers;
    g_InputEventHandlersType g_InputEventHandlers;
    bool g_HasFocus = false;
}

struct app_state {
    bool destroyRequested = false;
    bool initialized = false;
} g_AppState;

namespace App
{
    app_state* GetAppState()
    {
        return &g_AppState;
    }

    void OnAppCmd(int32_t command, App::CommandEventCallbackType callback)
    {
        g_CommandEventHandlers.insert({command, callback});
    }

    void OnInput(int32_t type, App::InputEventCallbackType callback)
    {
        g_InputEventHandlers.insert({type, callback});
    }

    bool HasFocus()
    {
        return g_HasFocus;
    }
}

namespace AppBackend
{
    void InjectInput(AppMotionAction action, float x, float y)
    {
        auto keyRange = g_InputEventHandlers.equal_range(action);
        for_each(keyRange.first, keyRange.second, [/*action, */x, y](g_InputEventHandlersType::value_type& callbackPair){
            callbackPair.second(callbackPair.first, x, y);
            //callback(1, 1.0f, 1.0f);
        });
    }

    void SetHasFocus(bool hasFocus = true)
    {
        g_HasFocus = hasFocus;
    }

    void SetInitialized(bool initialized = true)
    {
        App::GetAppState()->initialized = initialized;
    }
}

namespace {

    void PostFrameCallback();

    bool InitializeWindow(ANativeWindow* window)
    {
        /*if (!Graphics::Initialize(window))
        {
            LOGE("Unable to initialize Graphics!");
            return false;
        }

        AppBackend::SetHasFocus();
        AppBackend::SetInitialized();

        PostFrameCallback();

        Gui::CreateButton("Press Me", [&window](Gui::ButtonImpl* button){
            LOGI("Button pressed");
            Vulkan::Test(window);
        });

        return true;*/

        if (!Vulkan::Initialize(window))
        {
            LOGE("Unable to initialize Vulkan!");
            return false;
        }

        AppBackend::SetInitialized();

        return true;
    }

    void RenderOneFrame(long frameTimeNanos, void* data)
    {
        if (App::GetAppState()->destroyRequested != 0)
        {
            Vulkan::Destroy();
            g_AppState.initialized = false;
            return;
        }

        //Simulation here

        //Render here
        if (App::HasFocus())
        {
            Vulkan::Draw();
            PostFrameCallback();
        }
    }

    void PostFrameCallback()
    {
        auto choreographer = AChoreographer_getInstance();
        AChoreographer_postFrameCallback(choreographer, &RenderOneFrame, 0);
    }

}

extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnStart(JNIEnv* jenv, jobject obj)
{
    LOGI("nativeOnStart");

    return;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnResume(JNIEnv* jenv, jobject obj)
{
    LOGI("nativeOnResume");
    AppBackend::SetHasFocus();
    if (App::GetAppState()->initialized)
    {
        PostFrameCallback();
    }
    return;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnPause(JNIEnv* jenv, jobject obj)
{
    LOGI("nativeOnPause");
    AppBackend::SetHasFocus(false);
    return;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnStop(JNIEnv* jenv, jobject obj)
{
    LOGI("nativeOnStop");
    AppBackend::SetHasFocus(false);
    return;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnRestart(JNIEnv* jenv, jobject obj)
{
    LOGI("nativeOnRestart");
    AppBackend::SetHasFocus(true);
}


extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnDestroy(JNIEnv* jenv, jobject obj)
{
    LOGI("nativeOnDestroy");
    App::GetAppState()->destroyRequested = true;
}



extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeSetSurface(JNIEnv* jenv, jobject obj, jobject surface)
{
    if (surface != 0)
    {
        ANativeWindow* window = ANativeWindow_fromSurface(jenv, surface);
        LOGI("Got window %p", window);
        if (App::GetAppState()->initialized)
        {
            Vulkan::ReAcquireSurface(window);
        }
        else if (InitializeWindow(window))
        {
            AppBackend::SetHasFocus();
            PostFrameCallback();
        }
    }
    else
    {
        LOGI("Releasing window");
        Vulkan::ReleaseSurface();
    }

    return;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnInput(JNIEnv* jenv, jobject obj, jint action, jfloat x, jfloat y)
{
    LOGI("Got input %d %.1f %.1f", action, x, y);
    AppBackend::InjectInput((AppMotionAction)action, x, y);
}


extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeOnConfigurationChanged(JNIEnv* jenv)
{
    LOGI("Configuration Changed");
}
