#ifndef TUTORIAL_PARTICLE_SYSTEM_HPP
#define TUTORIAL_PARTICLE_SYSTEM_HPP

#include <vulkan/vulkan_raii.hpp>
using vk::raii::Semaphore;
using vk::raii::Queue;

#include <runtime/application.hpp>
using Runtime::Application;

#include <graphics/core.hpp>
#include <graphics/commandBuffer.hpp>
#include <graphics/models.hpp>
#include <graphics/pipeline.hpp>
using Graphics::Models::Particle;

#include <chrono>
#include <random>

#include <vector>
using std::vector;

namespace Graphics {
    class TutorialParticleSystem {
        public:
            TutorialParticleSystem(uint32_t width, uint32_t height, uint32_t particleCount);
            TutorialParticleSystem(std::nullptr_t) noexcept;

            TutorialParticleSystem& operator=(std::nullptr_t) noexcept;

            void allocateBuffers();
            void updateUniformBuffer(uint32_t frameIndex);
            void recordComputePass(const Pipeline &pipeline, uint32_t frameIndex);
            void executeComputePass(const Semaphore &semaphore, const uint64_t &waitValue, const uint64_t &signalValue, uint32_t frameIndex);

            const vector<vk::raii::Buffer>& getStorageBuffers() const;
            const vector<vk::raii::Buffer>& getUniformBuffers() const;

            const vk::raii::Buffer& getStorageBuffer(uint32_t frameIndex) const;
            const vk::raii::Buffer& getUniformBuffer(uint32_t frameIndex) const;
            uint32_t getParticleCount() const;
        private:
            vector<vk::raii::Buffer>        shaderStorageBuffers;
            vector<vk::raii::DeviceMemory>  shaderStorageBuffersMemory;

            vector<vk::raii::Buffer>        uniformBuffers;
            vector<vk::raii::DeviceMemory>  uniformBuffersMemory;
            vector<void*>                   uniformBuffersMapped;
            
            CommandBuffer commandBuffer = nullptr;

            vector<Particle> particles;

            uint32_t width;
            uint32_t height;
            uint32_t count;

            void createUniformBuffers();
    };
}

#endif