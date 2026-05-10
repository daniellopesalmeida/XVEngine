#include "Renderer.h"
#include "utils/Logger.h"
#include <stdexcept>
#include "Vertex.h"
#include <renderer/Mesh.h>
#include <renderer/Material.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <scene/Light.h>

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
    CreateMsaaImage();
    CreatePipeline();
    CreateDefaultMaterial();

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
    config.samples = m_device.GetMsaaSamples();  // must match MSAA color image

    m_depthImage.Init(m_device, m_commandManager, config);
    m_depthImage.TransitionLayout(m_device, m_commandManager,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal);
}

void Renderer::CreateMsaaImage()
{
    ImageConfig config;
    config.width = m_swapchain.GetExtent().width;
    config.height = m_swapchain.GetExtent().height;
    config.format = m_swapchain.GetSurfaceFormat().format;
    // eTransientAttachment tells the driver this image never needs to be written
    // to VRAM — it can live entirely in tile memory on mobile/integrated GPUs.
    // On discrete GPUs the flag is simply ignored.
    config.usage = vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eTransientAttachment;
    config.aspectMask = vk::ImageAspectFlagBits::eColor;
    config.samples = m_device.GetMsaaSamples();

    m_msaaImage.Init(m_device, m_commandManager, config);
    m_msaaImage.TransitionLayout(m_device, m_commandManager,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);
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
    config.msaaSamples = m_device.GetMsaaSamples();  // pipeline must match images
    config.useVertexInput = true;
    config.vertexBinding = binding;
    config.vertexAttributes = { attrs.begin(), attrs.end() };
    config.pushConstantSize = sizeof(ObjectData);
    config.pushConstantStages = vk::ShaderStageFlagBits::eVertex;

    // Both set layouts: set 0 = frame UBO, set 1 = material textures
    config.descriptorSetLayouts = {
        m_descriptorManager.GetFrameLayout(),
        m_descriptorManager.GetMaterialLayout()
    };

    m_pipeline.Init(m_device, config);
}

void Renderer::CreateDefaultMaterial()
{
    // All slots empty — Material::Init fills each with its 1x1 fallback texture.
    // This ensures every draw call always has a valid set 1 bound, even for
    // objects like the debug cube that have no material assigned.
    m_defaultMaterial = std::make_unique<Material>();
    m_defaultMaterial->Init(
        m_device, m_commandManager,
        m_descriptorManager.GetMaterialPool(),
        m_descriptorManager.GetMaterialLayout(),
        MaterialDesc{});   // all paths empty → white diffuse, grey spec/gloss, flat normal

    Logger::Info("Default material created");
}

void Renderer::RecreateSwapchain()
{
    while (m_window->GetWidth() == 0 || m_window->GetHeight() == 0)
        glfwWaitEvents();

    WaitIdle();
    m_swapchain.Recreate(m_device, m_surface, *m_window);

    m_depthImage.Shutdown();
    m_msaaImage.Shutdown();
    CreateDepthImage();
    CreateMsaaImage();

    m_commandManager.RecreateSyncObjects(m_device, m_swapchain);
    m_needsResize = false;

    Logger::Info("Swapchain recreated: ",
        m_swapchain.GetExtent().width, "x", m_swapchain.GetExtent().height);
}

bool Renderer::BeginFrame(const RenderList& list)
{
    if (m_needsResize || m_window->WasResized())
    {
        m_window->ClearResized();
        RecreateSwapchain();
        return false;
    }

    auto& fence = m_commandManager.GetInFlightFence(m_currentFrame);
    auto& cmd = m_commandManager.GetCommandBuffer(m_currentFrame);

    if (m_device.GetDevice().waitForFences(*fence, true, std::numeric_limits<uint64_t>::max())
        != vk::Result::eSuccess)
        throw std::runtime_error("waitForFences failed");

    auto [result, imageIndex] = m_swapchain.GetSwapchain().acquireNextImage(
        std::numeric_limits<uint64_t>::max(),
        *m_commandManager.GetPresentCompleteSemaphore(m_currentFrame),
        nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) { RecreateSwapchain(); return false; }

    m_imageIndex = imageIndex;
    m_device.GetDevice().resetFences(*fence);

    cmd.reset();
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);

    // Transition the swapchain image to color attachment — it will be the resolve target
    vk::ImageMemoryBarrier2 swapBarrier{};
    swapBarrier.image = m_swapchain.GetImages()[m_imageIndex];
    swapBarrier.oldLayout = vk::ImageLayout::eUndefined;
    swapBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    swapBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    swapBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    swapBarrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    swapBarrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    swapBarrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    swapBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    swapBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &swapBarrier;
    cmd.pipelineBarrier2(depInfo);

    // MSAA color attachment — we render into this multisampled image
    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.imageView = *m_msaaImage.GetImageView();
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;  // transient, never stored
    colorAttachment.clearValue = vk::ClearColorValue{ 0.1f, 0.1f, 0.1f, 1.0f };
    // Resolve target: driver resolves MSAA → single-sample swapchain image at end of pass
    colorAttachment.resolveMode = vk::ResolveModeFlagBits::eAverage;
    colorAttachment.resolveImageView = *m_swapchain.GetImageViews()[m_imageIndex];
    colorAttachment.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;

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
    m_pipeline.Bind(cmd);

    // Fill FrameData UBO
    FrameData frameData{};
    frameData.view = list.view;
    frameData.proj = list.proj;
    frameData.cameraPos = glm::vec4(list.cameraPos, 0.f);
    frameData.ambientColorAndStrength = glm::vec4(
        list.ambientColor,
        list.ambientStrength);
    uint32_t lightCount = 0;
       for (const auto& light : list.lights)
       {
           if (lightCount >= MAX_LIGHTS) break;
    
           GpuLight& gl = frameData.lights[lightCount++];
    
           gl.colorAndIntensity = glm::vec4(light.color, light.intensity);
           gl.positionAndRange  = glm::vec4(light.position, light.range);
           gl.directionAndType  = glm::vec4(glm::normalize(light.direction),
                                            static_cast<float>(light.type));
    
           // Pre-compute cosines so the shader doesn't have to
           float cosInner = glm::cos(glm::radians(light.innerCone));
           float cosOuter = glm::cos(glm::radians(light.outerCone));
           gl.coneAngles  = glm::vec4(cosInner, cosOuter, 0.f, 0.f);
       }
       frameData.lightCount = lightCount;
    m_descriptorManager.UpdateFrameData(m_currentFrame, frameData);

    // Bind set 0 (frame UBO) — stays bound for the whole frame
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *m_pipeline.GetPipelineLayout(),
        0,
        *m_descriptorManager.GetFrameSet(m_currentFrame),
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
        // Bind set 1 — use object's material or fall back to default
        Material* mat = dc.material ? dc.material : m_defaultMaterial.get();
        mat->Bind(cmd, *m_pipeline.GetPipelineLayout(), 1);

        ObjectData pc{};
        pc.model = dc.transform;

        // Normal matrix = inverse-transpose of the upper-left 3x3 of model.
        // GLM mat3(mat4) extracts the upper-left 3x3. inverseTranspose does both ops.
        // mat[i] in GLM = column i — so we push columns, shader reconstructs columns.
        glm::mat3 nm = glm::inverseTranspose(glm::mat3(dc.transform));
        pc.normalCol0 = glm::vec4(nm[0], 0.f);   // column 0
        pc.normalCol1 = glm::vec4(nm[1], 0.f);   // column 1
        pc.normalCol2 = glm::vec4(nm[2], 0.f);   // column 2

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

    // After endRendering the resolve is complete. Transition swapchain image to present.
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

    cmd.end();

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
    m_defaultMaterial->Shutdown();
    m_defaultMaterial.reset();
    m_pipeline.Shutdown();
    m_msaaImage.Shutdown();
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