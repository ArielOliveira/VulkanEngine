#include <graphics/renderer.hpp>

namespace Graphics {
    Renderer& Renderer::getInstance() {
        static Renderer instance;

        return instance;
    }

    Renderer::Renderer() {
        const Core& core = Core::getInstance();

        swapChain = SwapChain(core.getSurface(), core.getPhysicalDevice(), core.getDevice());
        pipeline  = Pipeline(core.getDevice(), swapChain.getExtent(), swapChain.getSurfaceFormat(), core.findDepthFormat());
    }

    Renderer::~Renderer() {}
}
