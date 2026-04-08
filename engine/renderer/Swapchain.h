#pragma once
#include <renderer/VulkanIncludes.h>
#include<core/Window.h>
#include <renderer/Device.h>

class Swapchain 
{
public:
    void Init(Device& device, vk::raii::SurfaceKHR& surface, Window& window);
    void Recreate(Device& device, vk::raii::SurfaceKHR& surface, Window& window);
    void Shutdown();

    vk::raii::SwapchainKHR& GetSwapchain() { return m_swapchain; }
    std::vector<vk::raii::ImageView>& GetImageViews() { return m_imageViews; }
    std::vector<vk::Image>& GetImages() { return m_images; }
    vk::Extent2D GetExtent() { return m_extent; }
    vk::SurfaceFormatKHR GetSurfaceFormat() { return m_surfaceFormat; }

private:
    vk::raii::SwapchainKHR m_swapchain = nullptr;
    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_imageViews;
    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::Extent2D m_extent;

    void CreateSwapchain(Device& device, vk::raii::SurfaceKHR& surface, Window& window);
    void CreateImageViews(Device& device);
    void CleanupSwapchain();
    vk::SurfaceFormatKHR ChooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
    vk::PresentModeKHR ChoosePresentMode(std::vector<vk::PresentModeKHR> const& availableModes);
    vk::Extent2D ChooseExtent(vk::SurfaceCapabilitiesKHR const& capabilities, Window& window);
    uint32_t ChooseMinImageCount(vk::SurfaceCapabilitiesKHR const& capabilities);
};