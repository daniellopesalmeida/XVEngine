#pragma once
#include <scene/Scene.h>
#include <memory>
#include <string>
#include <unordered_map>

class SceneManager
{
public:
    void Init(Device& device, CommandManager& cmdManager);
    void Shutdown();

    //creates an empty scene and returns a ptr to it
    Scene* AddScene(const std::string& name);

    void LoadScene(const std::string& name);
    void UnloadCurrent();

    Scene* GetActiveScene() { return m_activeScene; }
    Scene* GetScene(const std::string& name);
    bool HasActiveScene()  const { return m_activeScene != nullptr; }

    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);

    RenderList BuildRenderList();

private:
    Device* m_device = nullptr;
    CommandManager* m_cmdManager = nullptr;

    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
    Scene* m_activeScene = nullptr;
    std::string m_activeSceneName;
};