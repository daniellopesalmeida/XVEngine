#pragma once
#include "core/Window.h"
#include <renderer/VulkanIncludes.h>
#include<renderer/Instance.h>
#include<renderer/Device.h>
#include<renderer/Swapchain.h>
#include <renderer/CommandManager.h>
#include <renderer/Image.h>
#include <renderer/Pipeline.h>

class Renderer 
{
public:
    void Init(Window& window);
    bool BeginFrame();
    void EndFrame();
    void DrawFrame();
    void Shutdown();

    void SetPreferredGPU(vk::PhysicalDeviceType type) { m_preferredGPUType = type; }

private:
    Window* m_window = nullptr;
    bool m_initialized = false;
    vk::PhysicalDeviceType m_preferredGPUType = vk::PhysicalDeviceType::eIntegratedGpu;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = 0;

    //destruction is reverse ordr
    Instance  m_instance;
    vk::raii::SurfaceKHR m_surface = nullptr;  //owned by Renderer, needs instance and window
    Device m_device;
    Swapchain m_swapchain;
    //Descriptors m_descriptors;
    CommandManager m_commandManager;
    Image m_depthImage;
    Pipeline m_pipeline;

    void CreateSurface();
    void CreatePipeline();
    void CreateDepthImage();
};