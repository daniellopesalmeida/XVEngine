#pragma once
#include <renderer/VulkanIncludes.h>

class Instance 
{
public:
    void Init();
    void Shutdown();

    vk::raii::Instance& GetInstance() { return m_instance; }
    vk::raii::DebugUtilsMessengerEXT& GetDebugMessenger() { return m_debugMessenger; }
    vk::raii::Context& GetContext() { return m_context; }

private:
    vk::raii::Context m_context;
    vk::raii::Instance m_instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
#else
    const bool m_enableValidationLayers = true;
#endif

    void CreateInstance();
    void SetupDebugMessenger();
    std::vector<const char*> GetRequiredExtensions();


};