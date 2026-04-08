#include "CommandManager.h"
#include "utils/Logger.h"
#include <stdexcept>

void CommandManager::Init(Device& device, Swapchain& swapchain)
{
    CreateCommandPool(device);
    CreateCommandBuffers(device);
    CreateSyncObjects(device, swapchain);
    Logger::Info("CommandManager initialized");
}

void CommandManager::Shutdown()
{
    Logger::Info("CommandManager shutdown");
    // vk::raii handles cleanup
}

void CommandManager::CreateCommandPool(Device& device)
{
    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = device.GetQueueIndex();

    m_commandPool = vk::raii::CommandPool(device.GetDevice(), poolInfo);
    Logger::Info("Command pool created");
}

void CommandManager::CreateCommandBuffers(Device& device)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.level              = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    m_commandBuffers = vk::raii::CommandBuffers(device.GetDevice(), allocInfo);
    Logger::Info("Command buffers created: ", MAX_FRAMES_IN_FLIGHT);
}

void CommandManager::CreateSyncObjects(Device& device, Swapchain& swapchain)
{
    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    // one per swapchain image
    m_renderFinishedSemaphores.clear();
    for (size_t i = 0; i < swapchain.GetImages().size(); i++)
        m_renderFinishedSemaphores.emplace_back(device.GetDevice(), semaphoreInfo);

    // one per frame in flight
    m_presentCompleteSemaphores.clear();
    m_inFlightFences.clear();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_presentCompleteSemaphores.emplace_back(device.GetDevice(), semaphoreInfo);
        m_inFlightFences.emplace_back(device.GetDevice(), fenceInfo);
    }

    Logger::Info("Sync objects created");
    Logger::Info("\t-RenderFinished semaphores: ", m_renderFinishedSemaphores.size());
    Logger::Info("\t-PresentComplete semaphores: ", m_presentCompleteSemaphores.size());
    Logger::Info("\t-In flight fences: ", m_inFlightFences.size());
}

std::unique_ptr<vk::raii::CommandBuffer> CommandManager::BeginSingleTimeCommands(Device& device)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool= m_commandPool;
    allocInfo.level= vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    auto commandBuffer = std::make_unique<vk::raii::CommandBuffer>(
        std::move(vk::raii::CommandBuffers(device.GetDevice(), allocInfo).front()));

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer->begin(beginInfo);

    return commandBuffer;
}

void CommandManager::EndSingleTimeCommands(Device& device, vk::raii::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers= &*commandBuffer;

    device.GetQueue().submit(submitInfo, nullptr);
    device.GetQueue().waitIdle();
}