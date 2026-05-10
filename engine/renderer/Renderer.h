#pragma once
#include <core/Window.h>
#include <renderer/VulkanIncludes.h>
#include <renderer/Instance.h>
#include <renderer/Device.h>
#include <renderer/Swapchain.h>
#include <renderer/CommandManager.h>
#include <renderer/Image.h>
#include <renderer/Pipeline.h>
#include <renderer/ObjectData.h>
#include <renderer/DescriptorManager.h>
#include <renderer/Material.h>
#include <scene/RenderList.h>
#include <memory>

class Renderer
{
public:
    void Init(Window& window);
    bool BeginFrame(const RenderList& list);
    void DrawFrame(const RenderList& list);
    void EndFrame();
    void Shutdown();

    void SetPreferredGPU(vk::PhysicalDeviceType type) { m_preferredGPUType = type; }

    Device& GetDevice() { return m_device; }
    CommandManager& GetCommandManager() { return m_commandManager; }
    DescriptorManager& GetDescriptorManager() { return m_descriptorManager; }

    void WaitIdle();

private:
    Window* m_window = nullptr;
    bool    m_initialized = false;
    bool    m_needsResize = false;
    vk::PhysicalDeviceType m_preferredGPUType = vk::PhysicalDeviceType::eDiscreteGpu;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = 0;

    // Declare in init order — destroyed in reverse
    Instance             m_instance;
    vk::raii::SurfaceKHR m_surface = nullptr;
    Device               m_device;
    Swapchain            m_swapchain;
    CommandManager       m_commandManager;
    DescriptorManager    m_descriptorManager;
    Image                m_depthImage;
    Pipeline             m_pipeline;

    // Fallback material — used when a draw call has no material assigned.
    // All slots are 1x1 fallback textures (white diffuse, grey specular/gloss, flat normal).
    std::unique_ptr<Material> m_defaultMaterial;

    void CreateSurface();
    void CreateDepthImage();
    void CreatePipeline();
    void CreateDefaultMaterial();
    void RecreateSwapchain();
};