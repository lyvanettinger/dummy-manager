cmake_minimum_required (VERSION 3.8)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Make sure DLL and EXE targets go to the same directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Output directory for static lib (.LIB)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for shared lib (.DLL)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for executables (.EXE)

project( DialogueBox LANGUAGES CXX )

# Enable to build shared libraries.
option(BUILD_SHARED_LIBS "Create shared libraries." OFF)

# Enable multithreaded builds
if( MSVC )
    add_compile_options(/MP)
endif()

project ("DialogueBox")

# Include sub-projects.
add_subdirectory("external/DirectXTex")

set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only" )

add_subdirectory("external/glfw")


add_subdirectory("DiaBolic")

# Set the startup project.
set_directory_properties( PROPERTIES
	VS_STARTUP_PROJECT "DiaBolic"
)