#pragma once
#include <renderer/Mesh.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct MeshInstance
{
    Mesh* mesh = nullptr;   //non-owning
    glm::vec3 position = { 0.f, 0.f, 0.f };
    glm::vec3 rotation = { 0.f, 0.f, 0.f };  //euler degrees
    glm::vec3 scale = { 1.f, 1.f, 1.f };

    glm::mat4 GetTransform() const
    {
        glm::mat4 t = glm::translate(glm::mat4(1.f), position);
        t = glm::rotate(t, glm::radians(rotation.x), { 1, 0, 0 });
        t = glm::rotate(t, glm::radians(rotation.y), { 0, 1, 0 });
        t = glm::rotate(t, glm::radians(rotation.z), { 0, 0, 1 });
        t = glm::scale(t, scale);
        return t;
    }
};