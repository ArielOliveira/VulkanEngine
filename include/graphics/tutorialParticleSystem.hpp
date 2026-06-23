#ifndef TUTORIAL_PARTICLE_SYSTEM_HPP
#define TUTORIAL_PARTICLE_SYSTEM_HPP

#include <vulkan/vulkan_raii.hpp>

#include <graphics/core.hpp>
#include <graphics/commandBuffer.hpp>
#include <graphics/models.hpp>
using Graphics::Models::Particle;

#include <chrono>
#include <random>

#include <vector>
using std::vector;

namespace Graphics {
    class TutorialParticleSystem {
        public:
            TutorialParticleSystem(uint32_t width, uint32_t height, uint32_t particleCount);

            void allocateBuffers();
            void updateUniformBuffer(uint32_t frameIndex);
        private:
            vector<vk::raii::Buffer>       shaderStorageBuffers;
            vector<vk::raii::DeviceMemory> shaderStorageBuffersMemory;

            vector<vk::raii::Buffer>       uniformBuffers;
            vector<vk::raii::DeviceMemory> uniformBuffersMemory;
            vector<void*>                  uniformBuffersMapped;

            vector<Particle> particles;

            uint32_t width;
            uint32_t height;
            uint32_t count;

            void TutorialParticleSystem::createUniformBuffers();
    };
}

#endif