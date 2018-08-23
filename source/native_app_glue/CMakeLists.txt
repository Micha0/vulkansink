cmake_minimum_required(VERSION 3.6)

project(native_app_glue)

add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.h
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(native_app_glue INTERFACE ${ANDROID_NDK}/sources/android/native_app_glue)
# Fixes: https://github.com/android-ndk/ndk/issues/381
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

