#include "Renderer.h"
#include "utils/Logger.h"
#include <stdexcept>

void Renderer::Init(Window& window)
{
    m_window = &window;

    m_instance.Init();
    CreateSurface();
    m_device.Init(m_instance, m_surface, m_preferredGPUType);
    m_swapchain.Init(m_device, m_surface, *m_window);
    m_commandManager.Init(m_device, m_swapchain);
    


    m_initialized = true;
    Logger::Info("Renderer initialized");
}

void Renderer::CreateSurface()
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*m_instance.GetInstance(), m_window->GetHandle(), nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");

    m_surface = vk::raii::SurfaceKHR(m_instance.GetInstance(), surface);
    Logger::Info("Window surface created");
}

void Renderer::BeginFrame()
{
    // TODO
}

void Renderer::EndFrame()
{
    // TODO
}

void Renderer::Shutdown()
{
    if (!m_initialized)
        return;

    m_device.GetDevice().waitIdle();
    m_initialized = false;
    Logger::Info("Renderer shutdown");
}