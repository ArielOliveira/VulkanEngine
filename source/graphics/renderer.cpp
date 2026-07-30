#include <graphics/renderer.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <runtime/tiny_obj_loader.h>

#include <unordered_map>

#include <engine/resourceEngine.hpp>

namespace Graphics {
    Renderer& Renderer::getInstance() {
        static Renderer instance;

        return instance;
    }

    Renderer::Renderer() {
        const Core& core = Core::getInstance();

        swapChain        = SwapChain(core.getSurface(), core.getPhysicalDevice(), core.getDevice());

        graphicsPipeline = Pipeline::createGraphicsPipeline(core.getDevice(), swapChain.getExtent(), swapChain.getSurfaceFormat(), core.findDepthFormat(), core.getMaxUsableSampleCount());
        //graphicsPipeline = Pipeline::createSimpleGraphicsPipeline(core.getDevice(), swapChain.getExtent(), swapChain.getSurfaceFormat());
        //graphicsPipeline = Pipeline::createGraphicsPipelinePointList(core.getDevice(), swapChain.getExtent(), swapChain.getSurfaceFormat(), core.findDepthFormat(), core.getMaxUsableSampleCount());
        //computePipeline  = Pipeline::createComputePipeline(core.getDevice());
        renderPass       = CommandBuffer(core.getGraphicsCommandPool(), &core.getGraphicsQueue(), vk::CommandBufferLevel::ePrimary, Models::MAX_FRAMES_IN_FLIGHT);

        colorResolve  = Texture::createColorResolve(swapChain);
        depthBuffer   = Texture::createDepthBuffer(swapChain);
        
        std::string modelPath   = (Runtime::FileHelper::getExecutablePath() / Engine::Paths::MODELS / "viking_room.glb").string();
        std::string texturePath = (Runtime::FileHelper::getExecutablePath() / Engine::Paths::TEXTURES / "viking_room.ktx2").string();
        model = ResourceHandle<Model>(Engine::ResourceManager::getInstance().load<Model>(modelPath));
        modelTexture = ResourceHandle<Engine::Resources::Texture>(Engine::ResourceManager::getInstance().load<Engine::Resources::Texture>(texturePath));
        
        //particleSystem = TutorialParticleSystem(swapChain.getExtent().width, swapChain.getExtent().height, 4096);

        //computePipeline.createComputeDescriptorPool(core.getDevice());
        //computePipeline.createComputeDescriptorSets(core.getDevice(), particleSystem.getUniformBuffers(), particleSystem.getStorageBuffers(), particleSystem.getParticleCount());
                               
        //for (uint32_t i = 0; i < Models::MAX_FRAMES_IN_FLIGHT; i++)
        //    particleSystem.recordComputePass(computePipeline, i);

        vk::SemaphoreTypeCreateInfo semaphoreCreateInfo { .semaphoreType = vk::SemaphoreType::eTimeline, .initialValue = 0};
        renderingSemaphore = vk::raii::Semaphore(core.getDevice(), { .pNext = &semaphoreCreateInfo });
        createUniformBuffers();

        //graphicsPipeline.createSimpleGraphicsDescriptorPool(core.getDevice());
        //graphicsPipeline.createSimpleGraphicsDescriptorSets(core.getDevice(), uniformBuffers);

        graphicsPipeline.createGraphicsDescriptorPool(core.getDevice());
        graphicsPipeline.createGraphicsDescriptorSets(core.getDevice(), uniformBuffers, { modelTexture.data().sampler, modelTexture.data().image.data().view, vk::ImageLayout::eShaderReadOnlyOptimal });

        createSyncObjects();
    }

    Renderer::~Renderer() {}

    void Renderer::createSyncObjects() {
        assert(inFlightFences.empty());
        
        for (size_t i = 0; i < Graphics::Models::MAX_FRAMES_IN_FLIGHT; i++) 
            inFlightFences.emplace_back(Core::getInstance().getDevice(), vk::FenceCreateInfo{});
    }

    void Renderer::createUniformBuffers() {
        std::array<uint32_t, 2> queueFamilyIndices = { Core::getInstance().getGraphicsQueueIndex(), Core::getInstance().getTransferQueueIndex() };

        for (size_t i = 0; i < Graphics::Models::MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DeviceSize bufferSize = sizeof(Graphics::Models::UniformBufferObject);
            auto [buffer, bufferMem] = Core::getInstance().createBuffer(
                bufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                Core::getInstance().hasDedicatedTransferQueue() ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                Core::getInstance().getTransferQueueIndex(),
                Core::getInstance().hasDedicatedTransferQueue() ? queueFamilyIndices.data()    : nullptr
            );

            uniformBuffers.emplace_back(std::move(buffer));
            uniformBuffersMemory.emplace_back(std::move(bufferMem));
            uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));
        }
    }

    void Renderer::updateUniformBuffer(uint32_t frameIndex) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time       = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Graphics::Models::UniformBufferObject ubo{
            .model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            .view  = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            .proj  = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChain.getExtent().width) / static_cast<float>(swapChain.getExtent().height), 0.1f, 10.0f)
        };

        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
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

        vk::PipelineStageFlags2 depthStageFlags = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        
        // Wrost function signature ever
        // TODO: See how this can be improved
        colorResolve.updateImageLayout(renderPass, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits2::eColorAttachmentWrite, vk::PipelineStageFlagBits2::eColorAttachmentOutput, frameIndex, true);
        depthBuffer.updateImageLayout(renderPass, vk::ImageLayout::eDepthAttachmentOptimal, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,  depthStageFlags, frameIndex, true);

        Texture::updateImageLayout(renderPass, colorBarrier, frameIndex);

        vk::ClearValue clearColor = vk::ClearColorValue(0.1f, 0.1f, 0.1f, 1.0f);
        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0.0f);

        vk::RenderingAttachmentInfo colorAttachmentInfo {
            .imageView          = colorResolve.getImageView(),
            .imageLayout        = vk::ImageLayout::eColorAttachmentOptimal,
            .resolveMode        = vk::ResolveModeFlagBits::eAverage,
            .resolveImageView   = swapChain.getImageView(imageIndex),
            .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp             = vk::AttachmentLoadOp::eClear,
            .storeOp            = vk::AttachmentStoreOp::eStore,
            .clearValue         = clearColor
        };

        vk::RenderingAttachmentInfo depthAttachmentInfo {
            .imageView      = *depthBuffer.getImageView(),
            .imageLayout    = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eDontCare,
            .clearValue     = clearDepth
        };

        /*vk::RenderingAttachmentInfo colorAttachmentInfo {
            .imageView          = swapChain.getImageView(imageIndex),
            .imageLayout        = vk::ImageLayout::eColorAttachmentOptimal,
            .resolveMode        = vk::ResolveModeFlagBits::eNone,
            .loadOp             = vk::AttachmentLoadOp::eClear,
            .storeOp            = vk::AttachmentStoreOp::eStore,
            .clearValue         = clearColor
        };*/

        vk::RenderingInfo renderingInfo = {
            .renderArea           = {.offset = {0, 0}, .extent = swapChain.getExtent()},
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &colorAttachmentInfo,
            .pDepthAttachment     = &depthAttachmentInfo
        };

        renderPass[frameIndex].beginRendering(renderingInfo);
        renderPass[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getInstance());
        renderPass[frameIndex].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.getExtent().width), static_cast<float>(swapChain.getExtent().height), 0.0f, 1.0f));
        renderPass[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.getExtent()));
        //renderPass[frameIndex].bindVertexBuffers(0, {*particleSystem.getStorageBuffer(frameIndex)}, {0});
        //renderPass[frameIndex].draw(particleSystem.getParticleCount(), 1, 0, 0);
        
        auto& mesh = model.data().scenes[0].data().meshes[0].first.data();

        renderPass[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getPipelineLayout(), 0, *graphicsPipeline.getDescriptorSet(frameIndex), nullptr);
    
        const VkBuffer& vertexBuffer = mesh.vertexBuffer;
        const VkBuffer& indexBuffer  = mesh.indexBuffer;
        
        renderPass[frameIndex].bindVertexBuffers(0, { vertexBuffer }, {0});

        if (mesh.indexCount > 0) {
            renderPass[frameIndex].bindIndexBuffer({ indexBuffer }, 0, mesh.indexType);
            renderPass[frameIndex].drawIndexed(mesh.indexCount, 1, 0, 0, 0);
        } else {
            renderPass[frameIndex].draw(mesh.vertexCount, 1, 0, 0);
        }

        //renderPass[frameIndex].bindIndexBuffer({ indexBuffer }, 0, mesh.indexType);
        //renderPass[frameIndex].drawIndexed(static_cast<uint32_t>(mesh.indexCount), 1, 0, 0, 0);
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
        
        auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, nullptr, inFlightFences[frameIndex]);
        vk::Result fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);

        if (fenceResult != vk::Result::eSuccess) 
            throw std::runtime_error("failed to wait for fence!");
        
        //auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, presentCompleteSemaphores[frameIndex], nullptr);

        if (result == vk::Result::eErrorOutOfDateKHR) {
            while(Runtime::Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

            std::cout << "Swap chain is out of date. Recreating..." << '\n';
            
            device.waitIdle(); // Wait until resource is no longer being used before recreating
            colorResolve = Texture::createColorResolve(swapChain);
            depthBuffer  = Texture::createDepthBuffer(swapChain);
            
            return;
        }

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        device.resetFences(*inFlightFences[frameIndex]);

        /*uint64_t computeWaitValue      = timelineValue;
        uint64_t computeSignalValue    = ++timelineValue;
        uint64_t graphicsWaitValue     = computeSignalValue;
        uint64_t graphicsSignalValue   = ++timelineValue;*/

        uint64_t graphicsWaitValue   =   timelineValue;
        uint64_t graphicsSignalValue = ++timelineValue;

        updateUniformBuffer(frameIndex);
        //particleSystem.updateUniformBuffer(frameIndex);
        //particleSystem.recordComputePass(computePipeline, frameIndex);
        //particleSystem.executeComputePass(renderingSemaphore, computeWaitValue, computeSignalValue, frameIndex);

        recordRenderPass(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        //vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eVertexInput);

        vk::TimelineSemaphoreSubmitInfo graphicsTimelineInfo {
            .waitSemaphoreValueCount   = 1,
            .pWaitSemaphoreValues      = &graphicsWaitValue,
            .signalSemaphoreValueCount = 1,
            .pSignalSemaphoreValues    = &graphicsSignalValue
        };

        const vk::SubmitInfo graphicsSubmitInfo {
            .pNext                = &graphicsTimelineInfo,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &*renderingSemaphore,
            .pWaitDstStageMask    = &waitDestinationStageMask,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &*renderPass[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &*renderingSemaphore
        };

        renderPass.submitOnIdle(graphicsSubmitInfo, nullptr, frameIndex);

        vk::SemaphoreWaitInfo waitInfo {
            .semaphoreCount = 1,
            .pSemaphores    = &*renderingSemaphore,
            .pValues        = &graphicsSignalValue
        };

        // Wait on CPU for graphics to complete before presenting
		auto semaphoreResult = device.waitSemaphores(waitInfo, UINT64_MAX);
		if (semaphoreResult != vk::Result::eSuccess)
			throw std::runtime_error("failed to wait for semaphore!");

        const vk::PresentInfoKHR presentInfo {
            .waitSemaphoreCount = 0,
            .pWaitSemaphores    = nullptr,
            .swapchainCount     = 1,
            .pSwapchains        = &*swapChain.getInstance(),
            .pImageIndices      = &imageIndex
        };

        result = graphicsQueue.presentKHR(presentInfo);

        switch (result) {
            case vk::Result::eSuccess: break;
            case vk::Result::eSuboptimalKHR:
            case vk::Result::eErrorOutOfDateKHR:
                while(Runtime::Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

                std::cout << "vk::Queue::presentKHR returned eSuboptimal or eErrorOutOfDate. Reacreating swap chain... !\n";

                device.waitIdle(); // Wait until resource is no longer being used before recreating
                swapChain    = SwapChain(swapChain, surface, physicalDevice, device);
                colorResolve = Texture::createColorResolve(swapChain);
                depthBuffer  = Texture::createDepthBuffer(swapChain);
                break;
            default:
                std::cout << "Queue returned an unexpected result !\n";
                break;        // an unexpected result is returned!
	    }

        frameIndex = (frameIndex + 1) % Models::MAX_FRAMES_IN_FLIGHT;
    }
}
