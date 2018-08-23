cmake_minimum_required(VERSION 3.6)

project(app)

add_library(app STATIC
	# ./application.cpp
	# ./application.h
	./java_application.cpp
	./java_application.h
	# ./main.cpp
	./graphics/egl.cpp
	./graphics/egl.h
	./graphics/wsi.cpp
	./graphics/wsi.h
	./graphics/gldebug.cpp
	./graphics/gldebug.h
	./graphics/shader.cpp
	./graphics/shader.h
	./graphics/graphics.cpp
	./graphics/graphics.h
	./graphics/gui.cpp
	./graphics/gui.h
	./graphics/texture.cpp
	./graphics/texture.h
	./graphics/vulkan-test.cpp
	./graphics/vulkan-test.h
	./graphics/vulkandebug.cpp
	./graphics/vulkandebug.h
	./utils/make_unique.h
	./utils/log.cpp
	./utils/log.h
	./utils/opengl.h
	./utils/timing.cpp
	./utils/timing.h
	./utils/vfs.cpp
	./utils/vfs.h
	./utils/fs_android.cpp
	./utils/fs_android.h
)

target_compile_definitions(app PUBLIC -DVK_USE_PLATFORM_ANDROID_KHR)

target_link_libraries(app profiler imgui android)
target_include_directories(app PUBLIC .)
target_include_directories(app PRIVATE ..)


