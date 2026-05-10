#pragma once
#include <core/Engine.h>
#include <scene/ObjectHandle.h>
#include <scene/Light.h>
#include <unordered_map>
#include <string>

class App
{
public:
    explicit App(Engine& engine);

    void Load();
    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);

private:
    Engine& m_engine;

    std::unordered_map<std::string, ObjectHandle> m_Objects;
    std::unordered_map<std::string, LightHandle>  m_Lights;

    void loadQuad(Scene* scene);
    void loadCube(Scene* scene);
    void loadObj(Scene* scene);
    void loadGround(Scene* scene);
};