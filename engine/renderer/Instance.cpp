#include "Instance.h"
#include "utils/Logger.h"
#include <stdexcept>

void Instance::Init()
{
    CreateInstance();
    SetupDebugMessenger();
    Logger::Info("Vulkan instance initialized");
}

void Instance::Shutdown()
{
    Logger::Info("Vulkan instance shutdown");
    // vk::raii handles cleanup automatically
}

void Instance::CreateInstance()
{
    constexpr vk::ApplicationInfo appInfo(
        "XVEngine",
        VK_MAKE_VERSION(1, 0, 0),
        "XVEngine",
        VK_MAKE_VERSION(1, 0, 0),
        vk::ApiVersion14);

    //get required layers
    std::vector<const char*> requiredLayers;
    if (m_enableValidationLayers)
        requiredLayers.assign(m_validationLayers.begin(), m_validationLayers.end());

    //check layer support
    auto layerProperties = m_context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
        [&layerProperties](auto const& requiredLayer) {
            return std::ranges::none_of(layerProperties,
                [requiredLayer](auto const& layerProperty) {
                    return strcmp(layerProperty.layerName, requiredLayer) == 0;
                });
        });
    if (unsupportedLayerIt != requiredLayers.end())
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));

    //get required extensions
    auto requiredExtensions = GetRequiredExtensions();
    auto extensionProperties = m_context.enumerateInstanceExtensionProperties();
    auto unsupportedExtIt = std::ranges::find_if(requiredExtensions,
        [&extensionProperties](auto const& requiredExtension) {
            return std::ranges::none_of(extensionProperties,
                [requiredExtension](auto const& extensionProperty) {
                    return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                });
        });
    if (unsupportedExtIt != requiredExtensions.end())
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedExtIt));

    //logger info
    Logger::Info("Available Vulkan extensions:");
    for (const auto& extension : extensionProperties)
        Logger::Info("\t"," -", extension.extensionName);

    vk::InstanceCreateInfo createInfo(
        {},
        &appInfo,
        static_cast<uint32_t>(requiredLayers.size()),
        requiredLayers.data(),
        static_cast<uint32_t>(requiredExtensions.size()),
        requiredExtensions.data() );

    m_instance = vk::raii::Instance(m_context, createInfo);
    Logger::Info("Vulkan instance created");
}

void Instance::SetupDebugMessenger()
{
    if (!m_enableValidationLayers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        vk::DebugUtilsMessengerCreateFlagsEXT{},
        severityFlags,
        messageTypeFlags,
        &DebugCallback );

    m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(createInfo);
    Logger::Info("Debug messenger created");
}

std::vector<const char*> Instance::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto     glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (m_enableValidationLayers)
        extensions.push_back(vk::EXTDebugUtilsExtensionName);

    return extensions;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Instance::DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
    vk::DebugUtilsMessageTypeFlagsEXT              type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        Logger::Error("Validation layer: ", pCallbackData->pMessage);
    else
        Logger::Info("Validation layer: ", pCallbackData->pMessage);

    return vk::False;
}