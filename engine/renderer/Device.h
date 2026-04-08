#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Instance.h>

class Device 
{
public:
    void Init(Instance& context, vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType);
    void Shutdown();

    vk::raii::Device& GetDevice() { return m_device; }
    vk::raii::PhysicalDevice& GetPhysicalDevice() { return m_physicalDevice; }
    vk::raii::Queue& GetQueue() { return m_queue; }
    uint32_t GetQueueIndex() { return m_queueIndex; }
    vk::SampleCountFlagBits GetMsaaSamples() { return m_msaaSamples; }

private:
    vk::PhysicalDeviceType m_preferredGPUType;
    vk::raii::PhysicalDevice m_physicalDevice = nullptr;
    vk::raii::Device m_device = nullptr;
    vk::raii::Queue m_queue = nullptr;
    uint32_t m_queueIndex = ~0;
    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;

    void PickPhysicalDevice(Instance& context, vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType);
    void CreateLogicalDevice();
    bool IsDeviceSuitable(vk::raii::PhysicalDevice const& device, vk::raii::SurfaceKHR& surface);
    vk::SampleCountFlagBits GetMaxUsableSampleCount();
};