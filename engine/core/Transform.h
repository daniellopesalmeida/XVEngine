#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct Transform
{
    glm::vec3 position = { 0.f, 0.f, 0.f };
    glm::vec3 rotation = { 0.f, 0.f, 0.f };  // euler degrees XYZ
    glm::vec3 scale = { 1.f, 1.f, 1.f };

    glm::mat4 GetMatrix() const
    {
        glm::mat4 translation =glm::translate(glm::mat4(1.f), position);

        glm::mat4 rotationX = glm::rotate(glm::mat4(1.f), glm::radians(rotation.x), { 1,0,0 });
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.f), glm::radians(rotation.y), { 0,1,0 });
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.f), glm::radians(rotation.z), { 0,0,1 });

        glm::mat4 rotationMatrix = rotationY * rotationX * rotationZ;

        glm::mat4 scaleMatrix =glm::scale(glm::mat4(1.f), scale);

        return translation * rotationMatrix * scaleMatrix;
    }
};