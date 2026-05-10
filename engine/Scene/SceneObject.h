#pragma once
#include <core/Transform.h>
#include <renderer/Mesh.h>
#include <renderer/Material.h>

// One renderable entity: mesh geometry + material shading + world transform
struct SceneObject
{
    Transform  transform;
    Mesh* mesh = nullptr;   // owned by Scene::m_meshes
    Material* material = nullptr;   // owned by Scene::m_materials — nullptr = fallback
    std::string name;
};