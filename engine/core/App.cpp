#include "App.h"
#include <iostream>
#include <utils/Logger.h>

void App::Run()
{
    Init();
    MainLoop();
    Shutdown();
}

void App::Init()
{
    m_window.Init(800, 600, "XVEngine");
    m_renderer.Init(m_window);

    Logger::Info("Appl Initialized");
}

void App::MainLoop()
{
    while (!m_window.ShouldClose())
    {
        m_window.PollEvents();
        if (!m_renderer.BeginFrame())
        {
            Logger::Warn("Renderer.BeginFrame retrns false!");
            continue;
        }
        m_renderer.DrawFrame();
        m_renderer.EndFrame();
    }
}

void App::Shutdown()
{
    m_renderer.Shutdown();
    m_window.Shutdown();

    Logger::Info("Appl Shutdown");
}