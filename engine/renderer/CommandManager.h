#pragma once
#include "VulkanIncludes.h"
#include "Device.h"
#include "Swapchain.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class CommandManager {
public:
    void Init(Device& device, Swapchain& swapchain);
    void Shutdown();

    vk::raii::CommandPool& GetCommandPool() { return m_commandPool; }
    std::vector<vk::raii::CommandBuffer>& GetCommandBuffers() { return m_commandBuffers; }
    vk::raii::CommandBuffer& GetCommandBuffer(uint32_t frameIndex) { return m_commandBuffers[frameIndex]; }

    vk::raii::Semaphore& GetPresentCompleteSemaphore(uint32_t frameIndex) { return m_presentCompleteSemaphores[frameIndex]; }
    vk::raii::Semaphore& GetRenderFinishedSemaphore(uint32_t imageIndex) { return m_renderFinishedSemaphores[imageIndex]; }
    vk::raii::Fence&     GetInFlightFence(uint32_t frameIndex) { return m_inFlightFences[frameIndex]; }

    // single time commands helper
    std::unique_ptr<vk::raii::CommandBuffer> BeginSingleTimeCommands(Device& device);
    void EndSingleTimeCommands(Device& device, vk::raii::CommandBuffer& commandBuffer);

private:
    vk::raii::CommandPool m_commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    std::vector<vk::raii::Semaphore> m_presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_inFlightFences;

    void CreateCommandPool(Device& device);
    void CreateCommandBuffers(Device& device);
    void CreateSyncObjects(Device& device, Swapchain& swapchain);
};