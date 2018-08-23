cmake_minimum_required(VERSION 3.6)

project(imgui)

add_library(imgui STATIC
	imgui.cpp
	imgui.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_internal.h
	imconfig.h
	stb_rect_pack.h
	stb_textedit.h
	stb_truetype.h
)
