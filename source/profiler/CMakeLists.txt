cmake_minimum_required(VERSION 3.6)

project(profiler CXX)

list(APPEND sources
    ./profiler.cpp
    ./profiler.h
    ./communications/packets.cpp
    ./communications/packets.h
    ./communications/socketclient.cpp
    ./communications/socketclient.h
)

if(NOT PROFILER)
    #TODO: set all sources to type header, instead of removing
	add_library(profiler INTERFACE)
	target_sources(profiler INTERFACE ${CMAKE_CURRENT_LIST_DIR}/profiler.h)
else()
	add_library(profiler STATIC ${sources})
	target_include_directories(profiler PRIVATE ..)
    target_compile_definitions(profiler INTERFACE USE_PROFILER=1)
endif()
