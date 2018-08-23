#include "wsi.h"
#include "utils/log.h"

//namespace {
//	const char *type[] = {"up  ", "down", "move"};
//
//	class WSIDelegate : public WSIWindow {
//	    void OnMouseEvent(eAction action, int16_t x, int16_t y, uint8_t btn)
//		{
//	        LOGI("Mouse: %s %d x %d Btn:%d\n", type[action], x, y, btn);
//	    }
//
//	    void OnKeyEvent(eAction action, eKeycode keycode)
//		{
//			LOGI("Key: %s keycode:%d\n", type[action], keycode);
//		}
//
//	    void OnTextEvent(const char *str)
//		{
//			LOGI("Text: %s\n", str);
//		}
//
//	    void OnMoveEvent(int16_t x, int16_t y)
//		{
//			LOGI("Window Move: x=%d y=%d\n", x, y);
//		}
//
//	    void OnResizeEvent(uint16_t width, uint16_t height)
//		{
//			LOGI("Window Resize: width=%4d height=%4d\n", width, height);
//		}
//
//	    void OnFocusEvent(bool hasFocus)
//		{
//			LOGI("Focus: %s\n", hasFocus ? "True" : "False");
//		}
//
//	    void OnTouchEvent(eAction action, float x, float y, uint8_t id)
//		{
//	        LOGI("Touch: %s %4.0f x %4.0f Finger id:%d\n", type[action], x, y, id);
//	    }
//
//	    void OnCloseEvent() { printf("Window Closing.\n"); }
//	};
//
//	WSIDelegate delegate;
//}

namespace {
    VkSurfaceKHR surface = 0;
    ANativeWindow* window = nullptr;

    void CreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* createInfo, const VkAllocationCallbacks* allocators, VkSurfaceKHR* surface)
    {
        auto func = (PFN_vkCreateAndroidSurfaceKHR) vkGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR");
        if (func != nullptr)
        {
            func(instance, createInfo, allocators, surface);
        }
    }
}

namespace WSI {
    uint32_t GetWidth()
    {
        return static_cast<uint32_t>(ANativeWindow_getWidth(window));
    }

    uint32_t GetHeight()
    {
        return static_cast<uint32_t>(ANativeWindow_getHeight(window));
    }

	bool Initialize(VkInstance instance, ANativeWindow* window_)
	{
        window = window_;

        VkAndroidSurfaceCreateInfoKHR android_createInfo = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = window,
        };

        CreateAndroidSurfaceKHR(instance, &android_createInfo, nullptr, &surface);

        LOGI("Vulkan Surface created\n");
        return true;
	}

    VkSurfaceKHR GetSurface()
    {
        return surface;
    }

    void Destroy(VkInstance instance)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = 0;
        window = nullptr;
    }
}