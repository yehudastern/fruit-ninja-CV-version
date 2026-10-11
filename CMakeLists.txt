# =============================================================================
# CMakeLists.txt - build configuration for CV Fruit Ninja.
#
# Typical usage (from the project root):
#   cmake -S . -B build                  # configure: generate build files in build/
#   cmake --build build --config Release # compile
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# Write build/compile_commands.json, which editors and tools (clangd,
# clang-tidy, VS Code IntelliSense) use to understand how each file is compiled.
# (The Visual Studio generator ignores this setting; Ninja/Makefiles honor it.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cam-desktop-fruit-ninja VERSION 1.0 LANGUAGES CXX)

# Require C++20 exactly: fail instead of silently falling back to an older
# standard if the compiler does not support it.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# setting compiler flags for different platforms
#   /W4, -Wall -Wextra -Wpedantic : turn on (almost) all useful warnings.
#   /O2, -O3                      : optimize for speed.
# NOTE: add_compile_options applies to every target created after it,
# including raylib below.
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# downloading and building raylib from source
# FetchContent clones the raylib git repository at configure time (needs an
# internet connection the first time) and adds it to this build, so there is
# nothing to install manually. GIT_TAG pins an exact version for reproducible
# builds.
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
)
FetchContent_MakeAvailable(raylib)

# All source files of the game. Remember to add new .cpp files here.
set(SOURCES
    src/main.cpp
    src/NetworkReceiver.cpp
    src/Blade.cpp
    src/Fruit.cpp
    src/FruitSpawner.cpp
    src/ParticleSystem.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Where the compiler looks for #include "..." headers.
# PRIVATE = these settings apply only to this target, not to anything that
# links against it.
# NOTE: OpenCV is never searched for (there is no find_package(OpenCV)), so
# ${OpenCV_INCLUDE_DIRS} and ${OpenCV_LIBS} are empty leftovers. The camera
# code lives in Python, so the C++ game does not need OpenCV.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# Linking against the `raylib` target also brings in its include directories
# and its own dependencies (OpenGL, etc.) automatically.
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    raylib
)

# linking libraries for Windows
# ws2_32 = the Winsock 2 library, needed by NetworkReceiver for UDP sockets.
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()
