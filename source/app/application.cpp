#include "application.h"
#include "utils/log.h"
#include "graphics/graphics.h"
#include <unordered_map>
#include <algorithm>
#include "sdk/sdk.h"
#include <ghsdk/lib/src/android/native/JniWrapper.h>
#include <ghsdk/lib/src/android/native/Reachability.h>
#include <ghsdk/lib/src/android/native/com_gamehouse_ghsdk_GameHouseSdkLib.h>

namespace {
    android_app* g_AppState = nullptr;
    typedef std::unordered_multimap<int32_t, App::CommandEventCallbackType > g_CommandEventHandlersType;
    typedef std::unordered_multimap<int32_t, App::InputEventCallbackType > g_InputEventHandlersType;
    g_CommandEventHandlersType g_CommandEventHandlers;
    g_InputEventHandlersType g_InputEventHandlers;
    bool g_HasFocus = false;
}

namespace App
{
    android_app* GetAppState()
    {
        return g_AppState;
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

namespace {
    //handle cmd
    static void AppCmdHandler(android_app* AppState, int32_t cmd)
    {
        auto keyRange = g_CommandEventHandlers.equal_range(cmd);
        for_each(keyRange.first, keyRange.second, [](g_CommandEventHandlersType::value_type& callbackPair){
            callbackPair.second(callbackPair.first);
        });
    }
    //handle input
    static int32_t InputHandler(android_app* AppState, AInputEvent* event)
    {
        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
        {
            int source = AInputEvent_getSource(event);
            if (source == AINPUT_SOURCE_TOUCHSCREEN)
            {
                float x = -FLT_MAX;
                float y = -FLT_MAX;
                int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
                switch(action){
                    case AMOTION_EVENT_ACTION_MOVE:
                    case AMOTION_EVENT_ACTION_DOWN:
                        x = AMotionEvent_getX(event, 0);
                        y = AMotionEvent_getY(event, 0);
                        LOGI("State pressed");
                        break;
                }
                auto keyRange = g_InputEventHandlers.equal_range(action);
                for_each(keyRange.first, keyRange.second, [/*action, */x, y](g_InputEventHandlersType::value_type& callbackPair){
                    callbackPair.second(callbackPair.first, x, y);
                    //callback(1, 1.0f, 1.0f);
                });
                return 1;
            }
        }
        return 0;
    }

    void InitializeWindow(ANativeWindow* window)
    {
        if (!Graphics::Initialize(window))
        {
            LOGE("Unable to initialize Graphics!");
            return;
        }

        Sdk::test();
    }

    void Mainloop()
    {
        while(1)
        {
            // Read all pending events.
            int ident;
            int events;
            android_poll_source* source;

            while((ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0)
            {
                // Process this event.
                if (source != NULL)
                {
                    source->process(g_AppState, source);
                }

                // Check if we are exiting.
                if (g_AppState->destroyRequested != 0)
                {
                    Graphics::Destroy();
                    return;
                }
            }

            //Simulation here

            //Render here
            if (App::HasFocus())
            {
                Graphics::Draw();
            }
        }
    }

    void Initialize(android_app* _app_state)
    {
        //setup
        g_AppState = _app_state;

        g_AppState->onAppCmd = AppCmdHandler;
        g_AppState->onInputEvent = InputHandler;

        App::OnAppCmd(APP_CMD_INIT_WINDOW, [](int32_t) {
            if (g_AppState->window != nullptr) {
                InitializeWindow(g_AppState->window);
            }
        });

        App::OnAppCmd(APP_CMD_TERM_WINDOW, [](int32_t) {
            Graphics::Destroy();
        });

        App::OnAppCmd(APP_CMD_GAINED_FOCUS, [](int32_t) {
            g_HasFocus = true;
        });

        App::OnAppCmd(APP_CMD_LOST_FOCUS, [](int32_t) {
            g_HasFocus = false;
        });

        App::OnAppCmd(APP_CMD_CONFIG_CHANGED, [](int32_t) {
            LOGI("Configuration Changed");
            Graphics::Destroy();
            InitializeWindow(g_AppState->window);
        });

        //Run mainloop
        Mainloop();
    }

    void InitializeGamehouseSdk(ANativeActivity* activity)
    {
        ghsdk::JniWrapper::init_NativeActivity (activity);
        JNIEnv *env = ghsdk::JniWrapper::getEnv();

//        JNINativeMethod natives_SinkActivity[] = {
//            {"SetReachabilityStatus", "(I)V", (void*)&Java_com_example_micha_openglsink_SinkActivity_SetReachabilityStatus}
//        };

        //TODO: register natives
        JNINativeMethod natives_GameHouseSdkLib[] = {
            {"WebViewOnClosed", "(IILjava/lang/String;Ljava/lang/String;JJ)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_WebViewOnClosed},
            {"ReachabilityUpdateStatus", "(I)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_ReachabilityUpdateStatus},
            {"InAppPurchaseOnPurchaseStarted", "(JJLjava/lang/String;DLjava/lang/String;)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_InAppPurchaseOnPurchaseStarted},
            {"InAppPurchaseOnPurchaseSucceeded", "(JJLjava/lang/String;DLjava/lang/String;)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_InAppPurchaseOnPurchaseSucceeded},
            {"InAppPurchaseOnPurchaseFailed", "(JJLjava/lang/String;DLjava/lang/String;I)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_InAppPurchaseOnPurchaseFailed},
            {"InAppPurchaseOnProductRestored", "(JJLjava/lang/String;DLjava/lang/String;Ljava/lang/String;)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_InAppPurchaseOnProductRestored},
            {"InAppPurchaseOnRestoreCompleted", "(JJ)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_InAppPurchaseOnRestoreCompleted},
            {"InAppPurchaseOnRestoreFailed", "(JJI)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_InAppPurchaseOnRestoreFailed},
            {"InAppPurchasePurchasedProductCallback", "(JJZILjava/lang/String;)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_InAppPurchasePurchasedProductCallback},
            {"LoggerSetLogLevel", "(I)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_LoggerSetLogLevel},
            {"LoggerGetLogLevel", "()I", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_LoggerGetLogLevel},
            {"LoggerLog", "(ILjava/lang/String;Ljava/lang/String;)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_LoggerLog},
            {"LoggerGetLogs", "(Ljava/lang/String;)Ljava/lang/String;", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_LoggerGetLogs},
            {"UrlCacheManagerClear", "()V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_UrlCacheManagerClear},            
            {"DevModeEnable", "()V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_DevModeEnable},
            {"DevModeOnGestureRecognized", "(J)V", (void*)&Java_com_gamehouse_ghsdk_GameHouseSdkLib_DevModeOnGestureRecognized},
        };
        jclass jGameHouseSdkLibCls = ghsdk::JniWrapper::findClass("com/gamehouse/ghsdk/GameHouseSdkLib");
        ghsdk::JniWrapper::checkException();
        int registerResult = env->RegisterNatives(jGameHouseSdkLibCls, natives_GameHouseSdkLib, 15);
        ghsdk::Logger::instance()->info ("vulkansink", "RegisterNatives: %d", registerResult);


        ghsdk::JniWrapper::checkException ();
        jclass jGameHouseSdkCls = ghsdk::JniWrapper::findClass("com/gamehouse/ghsdk/GameHouseSdk");
        ghsdk::JniWrapper::checkException ();
        jstring jstoreBase64Code = env->NewStringUTF ("DEADBEEF0FC0FFEE");
        jmethodID jSdkInitialize = env->GetStaticMethodID (jGameHouseSdkCls, "sdkInitialize", "(Landroid/app/Activity;Ljava/lang/String;)V");
        ghsdk::JniWrapper::checkException ();
        env->CallStaticVoidMethod (jGameHouseSdkCls, jSdkInitialize, activity->clazz, jstoreBase64Code);
        ghsdk::JniWrapper::checkException ();
        env->DeleteLocalRef (jstoreBase64Code);

        DetachCurrentThread();
    }
}

void android_main(android_app* _app_state)
{
    InitializeGamehouseSdk(_app_state->activity);

    Initialize(_app_state);
}

