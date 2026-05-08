#include "Engine.h"
#include <utils/Logger.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <thread>

void Engine::Init()
{
    m_window.Init(800, 600, "XVEngine");
    m_renderer.Init(m_window);
    m_sceneManager.Init(m_renderer.GetDevice(), m_renderer.GetCommandManager(), m_window.GetAspect());
    RegisterWindowCallbacks();
    Logger::Info("Engine initialized");
}

void Engine::Run(const std::function<void()>& load)
{
    load();  // Game::Load() — scene setup before first frame

    constexpr float fixedTimestep = 1.f / 60.f;
    constexpr int   targetFps = 60;
    constexpr auto  targetFrameTime = std::chrono::milliseconds(1000 / targetFps);

    float lag = 0.f;
    auto  prevTime = std::chrono::steady_clock::now();

    while (!m_window.ShouldClose())
    {
        auto  currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - prevTime).count();
        prevTime = currentTime;

        m_window.PollEvents();

        // Fixed update for physics later
        lag += deltaTime;
        while (lag >= fixedTimestep)
        {
            m_sceneManager.FixedUpdate(fixedTimestep);
            if (m_fixedCb) m_fixedCb(fixedTimestep);
            lag -= fixedTimestep;
        }

        //update camera, gameplay
        m_sceneManager.Update(deltaTime);
        if (m_updateCb) m_updateCb(deltaTime);

        //render
        if (m_sceneManager.HasActiveScene())
        {
            RenderList list = m_sceneManager.BuildRenderList();
            if (m_renderer.BeginFrame(list))
            {
                m_renderer.DrawFrame(list);
                m_renderer.EndFrame();
            }
        }

        //frame cap
        auto frameTime = std::chrono::steady_clock::now() - currentTime;
        if (frameTime < targetFrameTime)
            std::this_thread::sleep_for(targetFrameTime - frameTime);
    }
}

void Engine::Shutdown()
{
    m_renderer.WaitIdle();

    m_sceneManager.Shutdown();
    m_renderer.Shutdown();
    m_window.Shutdown();
    Logger::Info("Engine shutdown");
}

void Engine::RegisterWindowCallbacks()
{
    m_window.SetKeyCallback([this](int key, int action)
        {
            if (auto* scene = m_sceneManager.GetActiveScene())
                scene->GetCamera().OnKey(key, action);
        });

    m_window.SetMouseMoveCallback([this](double dx, double dy)
        {
            if (auto* scene = m_sceneManager.GetActiveScene())
                scene->GetCamera().OnMouseMove(dx, dy);
        });

    m_window.SetMouseButtonCallback([this](int button, int action)
        {
            if (auto* scene = m_sceneManager.GetActiveScene())
                scene->GetCamera().OnMouseButton(button, action);
        });

    m_window.SetResizeCallback([this](uint32_t width, uint32_t height)
        {
            if (height == 0) return;  //guard divideby zero when minimised
            float aspect = static_cast<float>(width) / static_cast<float>(height);
            if (auto* scene = m_sceneManager.GetActiveScene())
            {
                scene->GetCamera().SetAspect(aspect);
            }
        });
}