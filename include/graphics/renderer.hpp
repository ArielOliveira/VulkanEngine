#ifndef GRAPHICS_RENDERER_HPP
#define GRAPHICS_RENDERER_HPP

#include <vulkan/vulkan_raii.hpp>

#include <graphics/core.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/swapChain.hpp>
#include <graphics/commandBuffer.hpp>

#include <vector>

using std::vector;

namespace Graphics {
    class Renderer {
        public:
            static Renderer& getInstance();

            Renderer(const Renderer&) = delete;
            Renderer(Renderer&&) noexcept = delete;

            Renderer& operator=(const Renderer&) = delete;
            Renderer& operator=(Renderer&&) noexcept = delete;

            void createSyncObjects();

            void drawFrame();

            ~Renderer();
        private:
            SwapChain     swapChain  = nullptr;
            Pipeline      pipeline   = nullptr;
            CommandBuffer renderPass = nullptr;

            vector<vk::raii::Semaphore> presentCompleteSemaphores;
            vector<vk::raii::Semaphore> renderFinishedSemaphores;
            vector<vk::raii::Fence> inFlightFences;

            uint32_t frameIndex = 0;

            Renderer();
    };
}

#endif