#ifndef TUTORIAL_PARTICLE_SYSTEM_HPP
#define TUTORIAL_PARTICLE_SYSTEM_HPP

#include <vulkan/vulkan_raii.hpp>
using vk::raii::Semaphore;
using vk::raii::Queue;

#include <runtime/application.hpp>
using Runtime::Application;

#include <engine/factory.hpp>

#include <engine/resources.hpp>

#include <graphics/core.hpp>
#include <graphics/commandBuffer.hpp>
#include <graphics/models.hpp>
#include <graphics/pipeline.hpp>

using Engine::Factory::ArrayFTY::make_null_array;
using Engine::ResourceHandle;
using Engine::Resources::GPU::Buffer;

using Graphics::Models::MAX_FRAMES_IN_FLIGHT;
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

            const std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT>& getStorageBuffers() const;
            const std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT>& getUniformBuffers() const;

            const Buffer& getStorageBuffer(uint32_t frameIndex) const;
            const Buffer& getUniformBuffer(uint32_t frameIndex) const;
            uint32_t getParticleCount() const;
        private:
            std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> shaderStorageBuffers = make_null_array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT>();
            std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> uniformBuffers       = make_null_array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT>();
            
            CommandBuffer commandBuffer = nullptr;

            vector<Particle> particles;

            uint32_t width;
            uint32_t height;
            uint32_t count;

            void createUniformBuffers();
    };
}

#endif