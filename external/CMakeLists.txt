## FETCH CONTENTS
include(FetchContent)

FetchContent_Declare(
        SpdLog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_Declare(
        D3D12MemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
        GIT_TAG f128d39b7a95b4235bd228d231646278dc6c24b2
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SpdLog D3D12MemoryAllocator)

## SUB DIRECTORIES

add_subdirectory("DirectXTex")

set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only" )

add_subdirectory("glfw")

add_library(External INTERFACE)
target_link_libraries(External INTERFACE DirectXTex glfw spdlog::spdlog D3D12MemoryAllocator)
target_include_directories(External INTERFACE ${D3D12MemoryAllocator_SOURCE_DIR}/include)