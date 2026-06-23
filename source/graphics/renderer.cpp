#include <graphics/renderer.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <runtime/tiny_obj_loader.h>

#include <unordered_map>

namespace Graphics {
    Renderer& Renderer::getInstance() {
        static Renderer instance;

        return instance;
    }

    Renderer::Renderer() {
        const Core& core = Core::getInstance();

        swapChain        = SwapChain(core.getSurface(), core.getPhysicalDevice(), core.getDevice());
        graphicsPipeline = Pipeline::createGraphicsPipeline(core.getDevice(), swapChain.getExtent(), swapChain.getSurfaceFormat(), core.findDepthFormat(), core.getMaxUsableSampleCount());
        renderPass       = CommandBuffer(core.getGraphicsCommandPool(), &core.getGraphicsQueue(), vk::CommandBufferLevel::ePrimary, Models::MAX_FRAMES_IN_FLIGHT);

        colorResolve  = Texture::createColorResolve(swapChain);
        depthBuffer   = Texture::createDepthBuffer(swapChain);
        modelTexture  = Texture(std::string("viking_room.png"), 
                               vk::Format::eR8G8B8A8Srgb, 
                               vk::ImageTiling::eOptimal, 
                               vk::ImageAspectFlagBits::eColor,
                               vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();

        graphicsPipeline.createGraphicsDescriptorPool(core.getDevice());
        graphicsPipeline.createGraphicsDescriptorSets(core.getDevice(), uniformBuffers, { modelTexture.getSampler(), modelTexture.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal });

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

    void Renderer::loadModel() {
        tinyobj::attrib_t attrib;
        vector<tinyobj::shape_t> shapes;
        vector<tinyobj::material_t> materials;
        string                      err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, Models::MODEL_PATH.c_str()))
            throw std::runtime_error(err);

        std::unordered_map<Graphics::Models::Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Graphics::Models::Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0 - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                auto [it, inserted] = uniqueVertices.insert({vertex, static_cast<uint32_t>(vertices.size())});

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (inserted)  vertices.push_back(vertex);
                indices.push_back(it->second);
            }

            cout << vertices.size() << " vertices loaded." << '\n';
        }
    }

    void Renderer::createVertexBuffer() {
        std::array<uint32_t, 2> queueFamilyIndices = { Core::getInstance().getGraphicsQueueIndex(), Core::getInstance().getTransferQueueIndex() };
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        auto [stagingBuffer, stagingBufferMemory]  = Core::getInstance().createBuffer(
                bufferSize,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        std::tie(vertexBuffer, vertexBufferMemory) = Core::getInstance().createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eVertexBuffer       | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            Core::getInstance().hasDedicatedTransferQueue() ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
            Core::getInstance().getTransferQueueIndex(),
            Core::getInstance().hasDedicatedTransferQueue() ? queueFamilyIndices.data()    : nullptr
        );

        void* data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, vertices.data(), bufferSize);
        stagingBufferMemory.unmapMemory();

        CommandBuffer::singleTimeTransfer().copyBuffer(stagingBuffer, vertexBuffer, bufferSize).submit();
    }

    void Renderer::createIndexBuffer() {
        std::array<uint32_t, 2> queueFamilyIndices = { Core::getInstance().getGraphicsQueueIndex(), Core::getInstance().getTransferQueueIndex() };
        vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        auto [stagingBuffer, stagingBufferMemory]  = Core::getInstance().createBuffer(
                bufferSize,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        void *data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, indices.data(), (size_t)bufferSize);
        stagingBufferMemory.unmapMemory();

        std::tie(indexBuffer, indexBufferMemory) =  Core::getInstance().createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eIndexBuffer        | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            Core::getInstance().hasDedicatedTransferQueue() ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
            Core::getInstance().getTransferQueueIndex(),
            Core::getInstance().hasDedicatedTransferQueue() ? queueFamilyIndices.data()    : nullptr
        );

        CommandBuffer::singleTimeTransfer().copyBuffer(stagingBuffer, indexBuffer, bufferSize).submit();
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

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
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
        renderPass[frameIndex].bindVertexBuffers(0, *vertexBuffer, {0});
        renderPass[frameIndex].bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
        renderPass[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getPipelineLayout(), 0, *graphicsPipeline.getDescriptorSet(frameIndex), nullptr);
        renderPass[frameIndex].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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

        recordRenderPass(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        
        updateUniformBuffer(frameIndex);

        const vk::SubmitInfo submitInfo {
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
            .pWaitDstStageMask    = &waitDestinationStageMask,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &*renderPass[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]
        };
        
        renderPass.submitOnIdle(submitInfo, inFlightFences[frameIndex], frameIndex);

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
                while(Runtime::Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

                std::cout << "vk::Queue::presentKHR returned eSuboptimal or eErrorOutOfDate. Reacreating swap chain... !\n";

                device.waitIdle(); // Wait until resource is no longer being used before recreating
                swapChain   = SwapChain(swapChain, surface, physicalDevice, device);
                colorResolve = Texture::createColorResolve(swapChain);
                depthBuffer  = Texture::createDepthBuffer(swapChain);
                break;
            default:
                std::cout << "Queue returned an unexpected result !\n";
                break;        // an unexpected result is returned!
	    }

        frameIndex = (frameIndex + 1) % Graphics::Models::MAX_FRAMES_IN_FLIGHT;
    }
}
