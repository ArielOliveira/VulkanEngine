#ifndef GRAPHICS_RENDERER_HPP
#define GRAPHICS_RENDERER_HPP

#include <vulkan/vulkan_raii.hpp>

#include <graphics/core.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/swapChain.hpp>
#include <graphics/commandBuffer.hpp>
#include <graphics/tutorialParticleSystem.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <engine/resources.hpp>

using Engine::Resources::Model;
using Engine::Resources::Mesh;
using Engine::Resources::Image;
using Engine::Resources::Texture;
using Engine::ResourceHandle;

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
            void createUniformBuffers();
            
            void updateUniformBuffer(uint32_t frameIndex);
            void recordRenderPass(uint32_t imageIndex);

            void drawFrame();

            ~Renderer();
        private:
            SwapChain     swapChain        = nullptr;
            Pipeline      graphicsPipeline = nullptr;
            Pipeline      computePipeline  = nullptr;
            CommandBuffer renderPass       = nullptr;
            
            ResourceHandle<Image> colorResolve     = nullptr;
            ResourceHandle<Image> depthBuffer      = nullptr;

            TutorialParticleSystem particleSystem = nullptr;

            vk::raii::Semaphore renderingSemaphore = nullptr;
            vector<vk::raii::Fence> inFlightFences;

            vector<vk::raii::Buffer>       uniformBuffers;
            vector<vk::raii::DeviceMemory> uniformBuffersMemory;
            vector<void*>                  uniformBuffersMapped;

            ResourceHandle<Model>          model = nullptr;
            ResourceHandle<Texture> modelTexture = nullptr;

            uint32_t frameIndex    = 0;
            uint32_t timelineValue = 0;

            Renderer();
    };
}

#endif