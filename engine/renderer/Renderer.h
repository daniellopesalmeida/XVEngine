#pragma once
#include "core/Window.h"
#include <renderer/VulkanIncludes.h>
#include<renderer/Instance.h>
#include<renderer/Device.h>
#include<renderer/Swapchain.h>
#include <renderer/CommandManager.h>

class Renderer 
{
public:
    void Init(Window& window);
    void BeginFrame();
    void EndFrame();
    void Shutdown();

    void SetPreferredGPU(vk::PhysicalDeviceType type) { m_preferredGPUType = type; }

private:
    Window* m_window = nullptr;
    bool m_initialized = false;
    vk::PhysicalDeviceType m_preferredGPUType = vk::PhysicalDeviceType::eIntegratedGpu;

    Instance  m_instance;
    vk::raii::SurfaceKHR m_surface = nullptr;  //owned by Renderer, needs instance and window
    Device m_device;
    Swapchain m_swapchain;
    //Pipeline m_pipeline;
    //Image m_image;
    //Descriptors m_descriptors;
    CommandManager m_commandManager;

    void CreateSurface();
};