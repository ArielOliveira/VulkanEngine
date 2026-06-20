#include <graphics/renderer.hpp>

namespace Graphics {
    Renderer& Renderer::getInstance() {
        static Renderer instance;

        return instance;
    }

    Renderer::Renderer() {
        const Core& core = Core::getInstance();

        swapChain  = SwapChain(core.getSurface(), core.getPhysicalDevice(), core.getDevice());
        pipeline   = Pipeline(core.getDevice(), swapChain.getExtent(), swapChain.getSurfaceFormat(), core.findDepthFormat());
        renderPass = CommandBuffer(core.getGraphicsCommandPool(), &core.getGraphicsQueue(), vk::CommandBufferLevel::ePrimary, Models::MAX_FRAMES_IN_FLIGHT);

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

    void Renderer::drawFrame() {

        frameIndex = (frameIndex + 1) % Models::MAX_FRAMES_IN_FLIGHT;
    }
}
