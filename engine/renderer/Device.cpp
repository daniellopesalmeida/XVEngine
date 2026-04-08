#include "Device.h"
#include "utils/Logger.h"
#include <stdexcept>

const std::vector<const char*> k_requiredDeviceExtensions = {
    vk::KHRSwapchainExtensionName
};

void Device::Init(Instance& instance, vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType)
{
    vk::PhysicalDeviceType m_preferredGPUType=preferredType;
    PickPhysicalDevice(instance, surface,m_preferredGPUType);
    CreateLogicalDevice();
    Logger::Info("Device initialized");
}

void Device::Shutdown()
{
    Logger::Info("Device shutdown");
    //vk::raii handles cleanup
}

void Device::PickPhysicalDevice(Instance& instance, vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType)
{
    auto physicalDevices = instance.GetInstance().enumeratePhysicalDevices();

    if (physicalDevices.empty())
        throw std::runtime_error("No Vulkan-compatible GPUs found!");

    //logger info
    Logger::Info("Available suitable GPUs:");
    for (auto const& device : physicalDevices)
    {
        if (IsDeviceSuitable(device, surface))
        {
            auto props = device.getProperties();
            Logger::Info("  -", props.deviceName, " (Type: ",
                vk::to_string(props.deviceType), ")");
        }
    }

    //rate device
    auto rateDevice = [preferredType, this, &surface](vk::raii::PhysicalDevice const& device) -> int {
        int score = 0;
        auto props = device.getProperties();

        if (props.deviceType == preferredType) score += 1000;

        //discrete > integrated > others
        if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 500;
        else if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 100;

        //if device not suitable
        if (!this->IsDeviceSuitable(device, surface))
            score = 0;

        return score;
        };

    //select device
    auto it = std::ranges::max_element(physicalDevices,
        [&](auto const& a, auto const& b) {
            return rateDevice(a) < rateDevice(b);
        });

    if (it == physicalDevices.end() || rateDevice(*it) == 0)
        throw std::runtime_error("Failed to find a suitable GPU!");

    m_physicalDevice = *it;
    m_msaaSamples = GetMaxUsableSampleCount();

    //logger info
    Logger::Info("Selected GPU: ", m_physicalDevice.getProperties().deviceName, " (Type: ",
        vk::to_string(m_physicalDevice.getProperties().deviceType), ") " );
    Logger::Info("MSAA samples: ", vk::to_string(m_msaaSamples));
}

bool Device::IsDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR& surface)
{
    //check Vulkan 1.3
    bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;

    //check graphics queue
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    bool supportsGraphics = std::ranges::any_of(queueFamilies,
        [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

    //check extensions
    auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsExtensions = std::ranges::all_of(k_requiredDeviceExtensions,
        [&availableExtensions](auto const& required) {
            return std::ranges::any_of(availableExtensions,
                [required](auto const& available) {
                    return strcmp(available.extensionName, required) == 0;
                });
        });

    //check features
    auto features = physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >();

    bool supportsFeatures =
        features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
        features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

    return supportsVulkan1_3 && supportsGraphics && supportsExtensions && supportsFeatures;
}

void Device::CreateLogicalDevice()
{
    auto queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
    {
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            m_queueIndex = i;
            break;
        }
    }
    if (m_queueIndex == ~0u)
        throw std::runtime_error("Could not find a graphics queue family!");

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > featureChain;

    //dynamic rendering enable
    featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
    featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
    featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = true;
    

    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.queueFamilyIndex = m_queueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(k_requiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = k_requiredDeviceExtensions.data();

    m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);
    m_queue = vk::raii::Queue(m_device, m_queueIndex, 0);

    Logger::Info("Logical device created");
    Logger::Info("Graphics queue family index: ", m_queueIndex);
}

//gets max samplecount (TODO: choose samples)
vk::SampleCountFlagBits Device::GetMaxUsableSampleCount()
{
    auto properties = m_physicalDevice.getProperties();
    auto counts = properties.limits.framebufferColorSampleCounts &
        properties.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e64) return vk::SampleCountFlagBits::e64;
    if (counts & vk::SampleCountFlagBits::e32) return vk::SampleCountFlagBits::e32;
    if (counts & vk::SampleCountFlagBits::e16) return vk::SampleCountFlagBits::e16;
    if (counts & vk::SampleCountFlagBits::e8)  return vk::SampleCountFlagBits::e8;
    if (counts & vk::SampleCountFlagBits::e4)  return vk::SampleCountFlagBits::e4;
    if (counts & vk::SampleCountFlagBits::e2)  return vk::SampleCountFlagBits::e2;

    return vk::SampleCountFlagBits::e1;
}