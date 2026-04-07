#pragma once

#include "core/Window.h"

class Renderer
{
public:
    Renderer(Window& window);
    ~Renderer();

    void Init();
    void DrawFrame();
    void Cleanup();

private:
    Window& m_Window;
};