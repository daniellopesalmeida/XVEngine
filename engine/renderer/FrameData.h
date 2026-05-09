#pragma once
#include <glm/glm.hpp>

//per frame uniform buffer, bound at set 0, binding 0
//updated once per frame before any draw calls
struct FrameData
{
    glm::mat4 view;
    glm::mat4 proj;

    //world-space directional light
    glm::vec4 lightDir;     //xyz = direction (normalised, pointing TOWARD light), w = unused
    glm::vec4 lightColor;   //xyz = RGB, w = unused
    glm::vec4 cameraPos;    //xyz = world-space camera position, w = unused

    //x = ambientStrength, y = specularStrength, z = shininess, w = unused
    glm::vec4 lightParams;
};