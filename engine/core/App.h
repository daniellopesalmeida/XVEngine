#pragma once
#include "Window.h"
#include <renderer/Renderer.h>

class App {
public:
    App() = default;
    ~App() = default;

    void Run();

private:
    Window m_window;
    Renderer m_renderer;

    void Init();
    void MainLoop();
    void Shutdown();
};