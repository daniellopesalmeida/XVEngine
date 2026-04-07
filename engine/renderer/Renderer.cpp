#include "renderer/Renderer.h"
#include "core/utils/Logger.h"

Renderer::Renderer(Window& window)
    : m_Window(window)
{
}

Renderer::~Renderer() = default;

void Renderer::Init()
{
    Logger::Info("Renderer initialized");
}

void Renderer::DrawFrame()
{
    //TODO: vulkan logic
}

void Renderer::Cleanup()
{
    Logger::Info("Renderer cleaned up");
}