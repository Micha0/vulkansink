cmake_minimum_required(VERSION 3.6)

set(PROJECT vulkansink)

project(${PROJECT} C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
cmake_policy(SET CMP0002 NEW)

#--- enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

#--- Android setup
set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
set(CMAKE_ANDROID_STL_TYPE c++_static)
set(CMAKE_ANDROID_CPP_FEATURES rtti exceptions)

#--- keep everything here ..
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

#--- enable RTTI and Exceptions
set(LLVM_ENABLE_RTTI TRUE)
set(LLVM_ENABLE_EH TRUE)

#---

#--- add our code
# add_subdirectory(native_app_glue)
add_subdirectory(imgui)
add_subdirectory(profiler)
add_subdirectory(app)

add_library(${PROJECT} SHARED)

target_include_directories(app INTERFACE .)
target_link_libraries(${PROJECT} log vulkan GLESv2 EGL android OpenSLES imgui profiler app)

#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_example_micha_vulkansink_VulkanActivity_nativeOnPause")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_example_micha_vulkansink_VulkanActivity_nativeOnResume")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_example_micha_vulkansink_VulkanActivity_nativeOnStart")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_example_micha_vulkansink_VulkanActivity_nativeOnStop")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_example_micha_vulkansink_VulkanActivity_nativeSetSurface")