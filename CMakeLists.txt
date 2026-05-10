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

# tinyobjloader
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG v2.0.0rc13
)
FetchContent_MakeAvailable(tinyobjloader)

# stb (stb_image.h — header-only)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# Vulkan
find_package(Vulkan REQUIRED)

# Slang compiler (in vulkan directory)
find_program(SLANGC slangc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
if(NOT SLANGC)
    message(FATAL_ERROR "slangc not found - make sure VULKAN_SDK is set")
endif()
message(STATUS "Found slangc: ${SLANGC}")

# Shader compilation
file(GLOB_RECURSE SLANG_SOURCES "${CMAKE_SOURCE_DIR}/shaders/*.slang")

foreach(SLANG ${SLANG_SOURCES})
    get_filename_component(FILE_NAME ${SLANG} NAME_WE)
    set(SPV_VERT "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.vert.spv")
    set(SPV_FRAG "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.frag.spv")

    add_custom_command(
        OUTPUT  ${SPV_VERT} ${SPV_FRAG}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
        COMMAND ${SLANGC} ${SLANG} -target spirv -emit-spirv-directly -entry vertMain -o ${SPV_VERT}
        COMMAND ${SLANGC} ${SLANG} -target spirv -emit-spirv-directly -entry fragMain -o ${SPV_FRAG}
        DEPENDS ${SLANG}
        COMMENT "Compiling shader: ${FILE_NAME}.slang"
    )
    list(APPEND SPV_FILES ${SPV_VERT} ${SPV_FRAG})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_FILES})

file(GLOB_RECURSE SOURCES
    "main.cpp"
    "engine/**/*.cpp"
    "app/*.cpp"
    "app/**/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "engine/**/*.h"
    "app/*.h"
    "app/**/*.h"
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/app"
    "${CMAKE_SOURCE_DIR}/engine"
)

add_executable(XVEngine ${SOURCES} ${HEADERS})
add_dependencies(XVEngine Shaders)

target_include_directories(XVEngine PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/app
    ${Vulkan_INCLUDE_DIRS}
    ${tinyobjloader_SOURCE_DIR}
    ${stb_SOURCE_DIR}            # stb_image.h lives here
)

target_link_libraries(XVEngine PRIVATE
    glfw
    glm::glm
    Vulkan::Vulkan
    tinyobjloader
)

# Copy shaders to output directory after build
add_custom_command(TARGET XVEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:XVEngine>/shaders"
    COMMENT "Copying shaders to output directory"
)

# Copy models/ to output directory
if(EXISTS "${CMAKE_SOURCE_DIR}/models")
    add_custom_command(TARGET XVEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/models"
            "$<TARGET_FILE_DIR:XVEngine>/models"
        COMMENT "Copying models to output directory"
    )
endif()

# Copy textures/ to output directory
if(EXISTS "${CMAKE_SOURCE_DIR}/textures")
    add_custom_command(TARGET XVEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/textures"
            "$<TARGET_FILE_DIR:XVEngine>/textures"
        COMMENT "Copying textures to output directory"
    )
endif()

if(MSVC)
    target_compile_options(XVEngine PRIVATE
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /Zc:preprocessor
    )
endif()