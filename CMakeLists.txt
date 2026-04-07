cmake_minimum_required(VERSION 3.20)

project(XVEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

add_executable(XVEngine
    main.cpp
 "engine/core/Window.cpp" "engine/core/App.cpp" "engine/core/App.h" "engine/core/Window.h"  "engine/core/utils/Logger.h"  "engine/renderer/Renderer.h" "engine/renderer/Renderer.cpp")

 target_include_directories(XVEngine PRIVATE engine)

# Download and build GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)

FetchContent_MakeAvailable(glfw)

# Link GLFW
target_link_libraries(XVEngine PRIVATE glfw)

