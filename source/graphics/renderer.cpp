#include <graphics/renderer.hpp>

namespace Graphics {
    Renderer& Renderer::getInstance() {
        static Renderer instance;

        return instance;
    }

    Renderer::Renderer() {
        const Core& core = Core::getInstance();

        swapChain   = SwapChain(core.getSurface(), core.getPhysicalDevice(), core.getDevice());
        pipeline    = Pipeline(core.getDevice(), swapChain.getExtent(), swapChain.getSurfaceFormat(), core.findDepthFormat());
        renderPass  = CommandBuffer(core.getGraphicsCommandPool(), &core.getGraphicsQueue(), vk::CommandBufferLevel::ePrimary, Models::MAX_FRAMES_IN_FLIGHT);
        depthBuffer = Texture::createDepthBuffer(swapChain);

        createSyncObjects();
    }

    Renderer::~Renderer() {}

    void Renderer::createSyncObjects() {
        assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

        for (size_t i = 0; i < swapChain.getImageCount(); i++)
            renderFinishedSemaphores.emplace_back(Core::getInstance().getDevice(), vk::SemaphoreCreateInfo());
        
        for (size_t i = 0; i < Graphics::Models::MAX_FRAMES_IN_FLIGHT; i++) {
            presentCompleteSemaphores.emplace_back(Core::getInstance().getDevice(), vk::SemaphoreCreateInfo());
            inFlightFences.emplace_back(Core::getInstance().getDevice(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    void Renderer::recordRenderPass(uint32_t imageIndex) {
        renderPass[frameIndex].begin({});

        vk::ImageMemoryBarrier2 colorBarrier {
            .srcStageMask           = vk::PipelineStageFlagBits2::eColorAttachmentOutput, .srcAccessMask          = {},
            .dstStageMask           = vk::PipelineStageFlagBits2::eColorAttachmentOutput, .dstAccessMask          = vk::AccessFlagBits2::eColorAttachmentWrite,
            .oldLayout              = vk::ImageLayout::eUndefined,                        .newLayout              = vk::ImageLayout::eColorAttachmentOptimal,
            .srcQueueFamilyIndex    = vk::QueueFamilyIgnored,                             .dstQueueFamilyIndex    = vk::QueueFamilyIgnored,
            .image                  = swapChain.getImage(imageIndex),
            .subresourceRange       = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
        };

        // Wrost function signature ever
        // TODO: See how this can be improved
        vk::PipelineStageFlags2 depthStageFlags = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        depthBuffer.updateImageLayout(renderPass, vk::ImageLayout::eDepthAttachmentOptimal, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,  depthStageFlags, frameIndex, true);

        Texture::updateImageLayout(renderPass, colorBarrier, frameIndex);

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0.0f);

        vk::RenderingAttachmentInfo colorAttachmentInfo {
            .imageView      = swapChain.getImageView(imageIndex),
            .imageLayout    = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eStore,
            .clearValue     = clearColor
        };

        vk::RenderingAttachmentInfo depthAttachmentInfo {
            .imageView      = *depthBuffer.getImageView(),
            .imageLayout    = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eDontCare,
            .clearValue     = clearDepth
        };

        vk::RenderingInfo renderingInfo = {
            .renderArea           = {.offset = {0, 0}, .extent = swapChain.getExtent()},
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &colorAttachmentInfo,
            .pDepthAttachment     = &depthAttachmentInfo
        };

        renderPass[frameIndex].beginRendering(renderingInfo);
        renderPass[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getInstance());
        renderPass[frameIndex].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.getExtent().width), static_cast<float>(swapChain.getExtent().height), 0.0f, 1.0f));
        renderPass[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.getExtent()));
        renderPass[frameIndex].endRendering();

        colorBarrier.srcStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput; colorBarrier.dstStageMask  = vk::PipelineStageFlagBits2::eBottomOfPipe;
        colorBarrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;         colorBarrier.dstAccessMask = {};
        colorBarrier.oldLayout     = vk::ImageLayout::eAttachmentOptimal;                colorBarrier.newLayout     = vk::ImageLayout::ePresentSrcKHR;

        Texture::updateImageLayout(renderPass, colorBarrier, frameIndex);
    }

    void Renderer::drawFrame() {
        const vk::raii::PhysicalDevice& physicalDevice = Core::getInstance().getPhysicalDevice();
        const vk::raii::Device& device                 = Core::getInstance().getDevice();
        const vk::raii::SurfaceKHR& surface            = Core::getInstance().getSurface();
        const vk::raii::Queue& graphicsQueue           = Core::getInstance().getGraphicsQueue();

        vk::Result fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
        
        if (fenceResult != vk::Result::eSuccess) 
            throw std::runtime_error("failed to wait for fence!");
        
        auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, presentCompleteSemaphores[frameIndex], nullptr);

        if (result == vk::Result::eErrorOutOfDateKHR) {
            while(Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

            std::cout << "Swap chain is out of date. Recreating..." << '\n';
            
            device.waitIdle(); // Wait until resource is no longer being used before recreating
            swapChain   = SwapChain(swapChain, surface, physicalDevice, device);
            depthBuffer = Texture::createDepthBuffer(swapChain);
            
            return;
        }

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        device.resetFences(*inFlightFences[frameIndex]);

        recordRenderPass(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        
        //updateUniformBuffer(frameIndex);

        const vk::SubmitInfo submitInfo {
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
            .pWaitDstStageMask    = &waitDestinationStageMask,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &*renderPass[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]
        };
        
        renderPass.dispatch(submitInfo, inFlightFences[frameIndex], frameIndex, true);

        const vk::PresentInfoKHR presentInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
            .swapchainCount     = 1,
            .pSwapchains        = &*swapChain.getInstance(),
            .pImageIndices      = &imageIndex
        };

        result = graphicsQueue.presentKHR(presentInfo);

        switch (result) {
            case vk::Result::eSuccess: break;
            case vk::Result::eSuboptimalKHR:
            case vk::Result::eErrorOutOfDateKHR:
                while(Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

                std::cout << "vk::Queue::presentKHR returned eSuboptimal or eErrorOutOfDate. Reacreating swap chain... !\n";

                device.waitIdle(); // Wait until resource is no longer being used before recreating
                swapChain   = SwapChain(swapChain, surface, physicalDevice, device);
                depthBuffer = Texture::createDepthBuffer(swapChain);
                break;
            default:
                std::cout << "Queue returned an unexpected result !\n";
                break;        // an unexpected result is returned!
	    }

        frameIndex = (frameIndex + 1) % Graphics::Models::MAX_FRAMES_IN_FLIGHT;
    }
}
