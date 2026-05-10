#pragma once
#include <core/Engine.h>
#include <scene/ObjectHandle.h>

class App
{
public:
    explicit App(Engine& engine);

    void Load();
    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);

private:
    Engine& m_engine;

    //handles to objects
    //ObjectHandle m_objectHandle;
    std::unordered_map<std::string, ObjectHandle> m_Objects;

    void loadQuad(Scene* scene);
    void loadCube(Scene* scene);
    void loadObj(Scene* scene);
    void loadGround(Scene* scene);
};