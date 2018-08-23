#pragma once

//#include <android_native_app_glue.h>
#include <functional>

struct app_state;


enum AppMotionAction {
    AMOTION_EVENT_ACTION_DOWN = 0,
    AMOTION_EVENT_ACTION_UP = 1,
    AMOTION_EVENT_ACTION_MOVE = 2
};


namespace App
{
    typedef std::function<void(int32_t command)> CommandEventCallbackType;
    typedef std::function<void(int32_t type, float x, float y)> InputEventCallbackType;

    app_state* GetAppState();
    void OnAppCmd(int32_t command, App::CommandEventCallbackType callback);
    void OnInput(int32_t type, App::InputEventCallbackType callback);
}
