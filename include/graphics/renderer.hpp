#ifndef GRAPHICS_RENDERER_HPP
#define GRAPHICS_RENDERER_HPP

#include <vulkan/vulkan_raii.hpp>

#include <graphics/core.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/swapChain.hpp>
#include <graphics/commandBuffer.hpp>
#include <graphics/texture.hpp>

#include <glm/gtc/matrix_transform.hpp>

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
            void loadModel();
            void createVertexBuffer();
            void createIndexBuffer();
            void createUniformBuffers();
            
            void updateUniformBuffer(uint32_t frameIndex);
            void recordRenderPass(uint32_t imageIndex);

            void drawFrame();

            ~Renderer();
        private:
            SwapChain     swapChain    = nullptr;
            Pipeline      pipeline     = nullptr;
            CommandBuffer renderPass   = nullptr;
            Texture       depthBuffer  = nullptr;
            Texture       modelTexture = nullptr;

            vector<vk::raii::Semaphore> presentCompleteSemaphores;
            vector<vk::raii::Semaphore> renderFinishedSemaphores;
            vector<vk::raii::Fence> inFlightFences;

            vector<Graphics::Models::Vertex> vertices;
            vector<uint32_t> indices;

            vector<vk::raii::Buffer>       uniformBuffers;
            vector<vk::raii::DeviceMemory> uniformBuffersMemory;
            vector<void*>                  uniformBuffersMapped;

            vk::raii::Buffer vertexBuffer                                 = nullptr;
            vk::raii::Buffer indexBuffer                                  = nullptr;
            vk::raii::DeviceMemory vertexBufferMemory                     = nullptr;
            vk::raii::DeviceMemory indexBufferMemory                      = nullptr;

            uint32_t frameIndex = 0;

            Renderer();
    };
}

#endif