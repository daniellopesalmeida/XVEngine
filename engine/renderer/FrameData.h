#pragma once
#include <glm/glm.hpp>

//per frame uniform buffer, bound at set 0, binding 0
//separate view/proj
struct FrameData
{
    glm::mat4 view;
    glm::mat4 proj;
};