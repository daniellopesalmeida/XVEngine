cmake_minimum_required(VERSION 3.20)
project(XVEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Vulkan
find_package(Vulkan REQUIRED)

# source and header collection
file(GLOB_RECURSE SOURCES
    "main.cpp"
    "engine/**/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "engine/**/*.h"
)

add_executable(XVEngine ${SOURCES} ${HEADERS})

target_include_directories(XVEngine PRIVATE
    ${CMAKE_SOURCE_DIR}          # #include for main.cpp
    ${CMAKE_SOURCE_DIR}/engine   
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(XVEngine PRIVATE
    glfw
    glm::glm
    Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(XVEngine PRIVATE
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /Zc:preprocessor
    )
endif()