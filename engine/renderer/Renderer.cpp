#include "Renderer.h"
#include "utils/Logger.h"
#include <stdexcept>
#include "Vertex.h"
#include <renderer/Mesh.h>
#include <glm/gtc/matrix_inverse.hpp>

void Renderer::Init(Window& window)
{
    m_window = &window;

    m_instance.Init();
    CreateSurface();
    m_device.Init(m_instance, m_surface, m_preferredGPUType);
    m_swapchain.Init(m_device, m_surface, *m_window);
    m_commandManager.Init(m_device, m_swapchain);
    m_descriptorManager.Init(m_device, m_commandManager);
    CreateDepthImage();
    CreatePipeline();

    m_initialized = true;
    Logger::Info("Renderer initialized");
}

void Renderer::CreateSurface()
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*m_instance.GetInstance(), m_window->GetHandle(), nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");

    m_surface = vk::raii::SurfaceKHR(m_instance.GetInstance(), surface);
    Logger::Info("Window surface created");
}

void Renderer::CreateDepthImage()
{
    ImageConfig config;
    config.width = m_swapchain.GetExtent().width;
    config.height = m_swapchain.GetExtent().height;
    config.format = vk::Format::eD32Sfloat;
    config.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    config.aspectMask = vk::ImageAspectFlagBits::eDepth;
    config.samples = vk::SampleCountFlagBits::e1;

    m_depthImage.Init(m_device, m_commandManager, config);
    m_depthImage.TransitionLayout(m_device, m_commandManager,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal);
}

void Renderer::CreatePipeline()
{
    auto binding = Vertex::GetBindingDescription();
    auto attrs = Vertex::GetAttributeDescriptions();

    PipelineConfig config;
    config.vertShaderPath = "shaders/shader.vert.spv";
    config.fragShaderPath = "shaders/shader.frag.spv";
    config.colorAttachmentFormats = { m_swapchain.GetSurfaceFormat().format };
    config.depthAttachmentFormat = vk::Format::eD32Sfloat;
    config.msaaSamples = vk::SampleCountFlagBits::e1;
    config.useVertexInput = true;
    config.vertexBinding = binding;
    config.vertexAttributes = { attrs.begin(), attrs.end() };
    config.pushConstantSize = sizeof(ObjectData);
    config.pushConstantStages = vk::ShaderStageFlagBits::eVertex;
    config.descriptorSetLayouts = { m_descriptorManager.GetLayout() };

    m_pipeline.Init(m_device, config);
}

void Renderer::RecreateSwapchain()
{
    while (m_window->GetWidth() == 0 || m_window->GetHeight() == 0)
        glfwWaitEvents();

    WaitIdle();

    m_swapchain.Recreate(m_device, m_surface, *m_window);

    m_depthImage.Shutdown();
    CreateDepthImage();

    m_commandManager.RecreateSyncObjects(m_device, m_swapchain);

    m_needsResize = false;
    Logger::Info("Swapchain recreated: ",
        m_swapchain.GetExtent().width, "x", m_swapchain.GetExtent().height);
}

bool Renderer::BeginFrame(const RenderList& list)
{
    //react to a pending resize before doing anything with the swapchain
    if (m_needsResize || m_window->WasResized())
    {
        m_window->ClearResized();
        RecreateSwapchain();
        return false;
    }

    auto& fence = m_commandManager.GetInFlightFence(m_currentFrame);
    auto& cmd = m_commandManager.GetCommandBuffer(m_currentFrame);

    if (m_device.GetDevice().waitForFences(*fence, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
        throw std::runtime_error("waitForFences failed");

    //acquire swapchain image
    auto [result, imageIndex] = m_swapchain.GetSwapchain().acquireNextImage(
        std::numeric_limits<uint64_t>::max(),
        *m_commandManager.GetPresentCompleteSemaphore(m_currentFrame),
        nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateSwapchain();
        return false;
    }

    m_imageIndex = imageIndex;
    m_device.GetDevice().resetFences(*fence);

    //begin command buffer
    cmd.reset();
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);

    //swapchain image to color attachment
    vk::ImageMemoryBarrier2 barrier{};
    barrier.image = m_swapchain.GetImages()[m_imageIndex];
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;
    cmd.pipelineBarrier2(depInfo);

    //begin dynamic rendering
    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.imageView = *m_swapchain.GetImageViews()[m_imageIndex];
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.clearValue = vk::ClearColorValue{ 0.1f, 0.1f, 0.1f, 1.0f };

    vk::RenderingAttachmentInfo depthAttachment{};
    depthAttachment.imageView = m_depthImage.GetImageView();
    depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.clearValue = vk::ClearDepthStencilValue{ 1.0f, 0 };

    vk::RenderingInfo renderingInfo{};
    renderingInfo.renderArea = vk::Rect2D{ {0, 0}, m_swapchain.GetExtent() };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    cmd.beginRendering(renderingInfo);

    //pipeline and set dynamic viewport/scissor
    m_pipeline.Bind(cmd);

    //fill FrameData UBO with camera + lighting
    FrameData frameData{};
    frameData.view = list.view;
    frameData.proj = list.proj;
    frameData.lightDir = glm::vec4(glm::normalize(list.lightDir), 0.f);
    frameData.lightColor = glm::vec4(list.lightColor, 0.f);
    frameData.cameraPos = glm::vec4(list.cameraPos, 0.f);
    frameData.lightParams = glm::vec4(
        list.ambientStrength,
        list.specularStrength,
        list.shininess,
        0.f);

    m_descriptorManager.UpdateFrameData(m_currentFrame, frameData);

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *m_pipeline.GetPipelineLayout(),
        0,
        *m_descriptorManager.GetDescriptorSet(m_currentFrame),
        nullptr);

    vk::Viewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
    viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    cmd.setViewport(0, viewport);

    vk::Rect2D scissor{ {0, 0}, m_swapchain.GetExtent() };
    cmd.setScissor(0, scissor);

    return true;
}

void Renderer::DrawFrame(const RenderList& list)
{
    auto& cmd = m_commandManager.GetCommandBuffer(m_currentFrame);

    for (const auto& dc : list.drawCalls)
    {
        //compute normal matrix CPU-side (inverse-transpose)
        //correctly handles non-uniform scaling without distorting normals
        glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(dc.transform));

        ObjectData pc{};
        pc.model = dc.transform;
        //pack mat3 into 3x vec4 (shader reads .xyz of each)
        pc.normalRow0 = glm::vec4(normalMat[0], 0.f);  // column 0 of mat3
        pc.normalRow1 = glm::vec4(normalMat[1], 0.f);  // column 1
        pc.normalRow2 = glm::vec4(normalMat[2], 0.f);  // column 2

        cmd.pushConstants<ObjectData>(
            *m_pipeline.GetPipelineLayout(),
            vk::ShaderStageFlagBits::eVertex,
            0, pc);

        cmd.bindVertexBuffers(0, *dc.mesh->GetVertexBuffer().GetBuffer(), vk::DeviceSize{ 0 });
        cmd.bindIndexBuffer(*dc.mesh->GetIndexBuffer().GetBuffer(), 0, vk::IndexType::eUint32);
        cmd.drawIndexed(dc.mesh->GetIndexCount(), 1, 0, 0, 0);
    }
}

void Renderer::EndFrame()
{
    auto& cmd = m_commandManager.GetCommandBuffer(m_currentFrame);
    auto& fence = m_commandManager.GetInFlightFence(m_currentFrame);

    cmd.endRendering();

    //color attachment 
    vk::ImageMemoryBarrier2 barrier{};
    barrier.image = m_swapchain.GetImages()[m_imageIndex];
    barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;
    cmd.pipelineBarrier2(depInfo);

    //Logger::Info("EndFrame: submitting");
    cmd.end();

    //submit
    vk::SemaphoreSubmitInfo waitSemInfo{};
    waitSemInfo.semaphore = *m_commandManager.GetPresentCompleteSemaphore(m_currentFrame);
    waitSemInfo.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

    vk::SemaphoreSubmitInfo signalSemInfo{};
    signalSemInfo.semaphore = *m_commandManager.GetRenderFinishedSemaphore(m_imageIndex);
    signalSemInfo.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;

    vk::CommandBufferSubmitInfo cmdInfo{};
    cmdInfo.commandBuffer = *cmd;

    vk::SubmitInfo2 submitInfo{};
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdInfo;

    m_device.GetQueue().submit2(submitInfo, *fence);

    // Present
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &*m_commandManager.GetRenderFinishedSemaphore(m_imageIndex);
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &*m_swapchain.GetSwapchain();
    presentInfo.pImageIndices = &m_imageIndex;

    auto result = m_device.GetQueue().presentKHR(presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        m_needsResize = true;

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::Shutdown()
{
    if (!m_initialized) return;

    WaitIdle();
    m_pipeline.Shutdown();
    m_depthImage.Shutdown();
    m_descriptorManager.Shutdown();
    m_commandManager.Shutdown();
    m_swapchain.Shutdown();
    m_initialized = false;
    Logger::Info("Renderer shutdown");
}

void Renderer::WaitIdle()
{
    m_device.GetDevice().waitIdle();
}