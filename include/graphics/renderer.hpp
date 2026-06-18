#ifndef GRAPHICS_RENDERER_HPP
#define GRAPHICS_RENDERER_HPP

#include <vulkan/vulkan_raii.hpp>

#include <graphics/core.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/swapChain.hpp>

namespace Graphics {
    class Renderer {
        public:
            static Renderer& getInstance();

            Renderer(const Renderer&) = delete;
            Renderer(Renderer&&) noexcept = delete;

            Renderer& operator=(const Renderer&) = delete;
            Renderer& operator=(Renderer&&) noexcept = delete;

            void drawFrame();

            ~Renderer();
        private:
            SwapChain swapChain = nullptr;
            Pipeline  pipeline  = nullptr;

            Renderer();

    };
}

#endif