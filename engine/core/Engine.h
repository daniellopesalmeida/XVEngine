#pragma once
#include <core/Window.h>
#include <core/SceneManager.h>
#include <renderer/Renderer.h>
#include <functional>

class Engine
{
public:
    void Init();
    void Run(const std::function<void()>& load);
    void Shutdown();

    // Callbacks set by Game
    void SetUpdateCallback(std::function<void(float)> cb) { m_updateCb = std::move(cb); }
    void SetFixedUpdateCallback(std::function<void(float)> cb) { m_fixedCb = std::move(cb); }

    SceneManager& GetSceneManager() { return m_sceneManager; }
    Renderer& GetRenderer() { return m_renderer; }
    Window& GetWindow() { return m_window; }

private:
    Window       m_window;
    Renderer     m_renderer;
    SceneManager m_sceneManager;

    std::function<void(float)> m_updateCb;
    std::function<void(float)> m_fixedCb;

    void RegisterWindowCallbacks();
};