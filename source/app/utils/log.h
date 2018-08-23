#pragma once
#include <android/log.h>
#include <string>

#define LOGTAG "VulkanSink"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOGTAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOGTAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOGTAG, __VA_ARGS__))


std::string ToHex(std::string in);

std::string ToHex(char* in, int size);