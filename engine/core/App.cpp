#include "App.h"
#include <iostream>
#include <core/utils/Logger.h>

App::App(int width, int height, const std::string& title)
    : m_Window(width, height, title),
    m_Renderer(m_Window)
{
    m_Renderer.Init();
}

App::~App() = default;

void App::Run()
{
    while (!m_Window.ShouldClose())
    {
        m_Window.PollEvents();

        m_Renderer.DrawFrame();
    }

    m_Renderer.Cleanup();

    Logger::Info("App closed!");
}