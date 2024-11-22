cmake_minimum_required (VERSION 3.8)

set( HEADER_FILES
	inc/camera.hpp
	inc/command_queue.hpp
	inc/dialogue_sample.hpp
	inc/dx12_helpers.hpp
	inc/d3dx12.h
	inc/glfw_app.hpp
	inc/renderer.hpp
	inc/resource_util.hpp
	inc/pipelines/geometry_pipeline.hpp
	inc/pipelines/ui_pipeline.hpp
)

set( SRC_FILES
	src/camera.cpp
	src/command_queue.cpp
	src/dialogue_sample.cpp
	src/dx12_helpers.cpp
	src/glfw_app.cpp
	src/main.cpp
	src/pch.h
	src/pch.cpp
	src/renderer.cpp
	src/resource_util.cpp
	src/pipelines/geometry_pipeline.cpp
	src/pipelines/ui_pipeline.cpp
)

set( SHADER_FILES
	shaders/uber_ps.hlsl
	shaders/uber_vs.hlsl
)

set_source_files_properties( ${SHADER_FILES} 
    PROPERTIES
        VS_SHADER_OBJECT_FILE_NAME "${CMAKE_SOURCE_DIR}/Data/Shaders/DiaBolic/%(Filename).cso"
        VS_SHADER_MODEL 6.0
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>>:ON>
        VS_SHADER_FLAGS $<$<OR:$<CONFIG:Debug>>:-Qembed_debug>
)

set_source_files_properties( shaders/uber_vs.hlsl 
    PROPERTIES 
        VS_SHADER_TYPE Vertex
)

set_source_files_properties( shaders/uber_ps.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Pixel
)

add_executable( DiaBolic WIN32
    ${HEADER_FILES}
    ${SRC_FILES}
    ${SHADER_FILES}
	src/pch.cpp
)

target_include_directories( DiaBolic
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries( DiaBolic
    PRIVATE DirectXTex
	PRIVATE GLFW
	PRIVATE d3d12.lib
	PRIVATE dxgi.lib
	PRIVATE D3DCompiler.lib
	PRIVATE glfw3.lib
	PRIVATE dxguid.lib
)

# Enable precompiled header files.
set_source_files_properties( ${SOURCE_FILES} 
    PROPERTIES 
        COMPILE_FLAGS /Yu"pch.h"
)

set_source_files_properties( src/pch.cpp
    PROPERTIES
        COMPILE_FLAGS /Yc"pch.h"
)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )