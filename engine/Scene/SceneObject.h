#pragma once
#include <core/Transform.h>
#include <renderer/Mesh.h>

//one mesh + one transform
struct SceneObject
{
    Transform    transform;
    Mesh* mesh = nullptr;   //mesh owned by Scene
    std::string  name;
};