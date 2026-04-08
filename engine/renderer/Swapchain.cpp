#include "Swapchain.h"
#include "utils/Logger.h"
#include <stdexcept>
#include <limits>
#include <algorithm>

void Swapchain::Init(Device& device, vk::raii::SurfaceKHR& surface, Window& window)
{
    CreateSwapchain(device, surface, window);
    CreateImageViews(device);
    Logger::Info("Swapchain initialized");
}

void Swapchain::Recreate(Device& device, vk::raii::SurfaceKHR& surface, Window& window)
{
    CleanupSwapchain();
    CreateSwapchain(device, surface, window);
    CreateImageViews(device);
    Logger::Info("Swapchain recreated");
}

void Swapchain::Shutdown()
{
    CleanupSwapchain();
    Logger::Info("Swapchain shutdown");
}

void Swapchain::CleanupSwapchain()
{
    m_imageViews.clear();
    m_swapchain = nullptr;
}

void Swapchain::CreateSwapchain(Device& device, vk::raii::SurfaceKHR& surface, Window& window)
{
    auto capabilities = device.GetPhysicalDevice().getSurfaceCapabilitiesKHR(*surface);
    auto availableFormats = device.GetPhysicalDevice().getSurfaceFormatsKHR(*surface);
    auto availableModes = device.GetPhysicalDevice().getSurfacePresentModesKHR(*surface);

    m_surfaceFormat = ChooseSurfaceFormat(availableFormats);
    auto presentMode = ChoosePresentMode(availableModes);
    m_extent = ChooseExtent(capabilities, window);
    uint32_t imageCount = ChooseMinImageCount(capabilities);

    vk::SwapchainCreateInfoKHR createInfo(
        {},                   // flags
        *surface,             // surface
        imageCount,           // minImageCount
        m_surfaceFormat.format,
        m_surfaceFormat.colorSpace,
        m_extent,
        1,                    // imageArrayLayers
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0, nullptr,           // queueFamilyIndexCount, pQueueFamilyIndices
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        true,                 // clipped
        nullptr               // oldSwapchain
    );

    m_swapchain = vk::raii::SwapchainKHR(device.GetDevice(), createInfo);
    m_images = m_swapchain.getImages();

    Logger::Info("Swapchain created: ", m_extent.width, "x", m_extent.height);
    Logger::Info("Swapchain format: ", vk::to_string(m_surfaceFormat.format));
    Logger::Info("Swapchain present mode: ", vk::to_string(presentMode));
    Logger::Info("Swapchain image count: ", m_images.size());
}

void Swapchain::CreateImageViews(Device& device)
{
    m_imageViews.clear();

    for (auto& image : m_images)
    {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = m_surfaceFormat.format;

        //components maping
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;

        //subresource range
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        createInfo.image = image;

        m_imageViews.emplace_back(device.GetDevice(), createInfo);
    }

    Logger::Info("Image views created: ", m_imageViews.size());
}

vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
    auto it = std::ranges::find_if(availableFormats,
        [](const auto& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    return it != availableFormats.end() ? *it : availableFormats[0];
}

vk::PresentModeKHR Swapchain::ChoosePresentMode(std::vector<vk::PresentModeKHR> const& availableModes)
{
    auto it = std::ranges::find_if(availableModes,
        [](const auto& mode) { return mode == vk::PresentModeKHR::eMailbox; });
    return it != availableModes.end() ? *it : vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::ChooseExtent(vk::SurfaceCapabilitiesKHR const& capabilities, Window& window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    return {
        std::clamp<uint32_t>(window.GetWidth(),  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(window.GetHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t Swapchain::ChooseMinImageCount(vk::SurfaceCapabilitiesKHR const& capabilities)
{
    uint32_t imageCount = std::max(3u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;
    return imageCount;
}