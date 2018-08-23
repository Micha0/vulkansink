#pragma once

    #include <android_native_app_glue.h>
#include <functional>

namespace App
{
    typedef std::function<void(int32_t command)> CommandEventCallbackType;
    typedef std::function<void(int32_t type, float x, float y)> InputEventCallbackType;

    android_app* GetAppState();
    void OnAppCmd(int32_t command, App::CommandEventCallbackType callback);
    void OnInput(int32_t type, App::InputEventCallbackType callback);
}
