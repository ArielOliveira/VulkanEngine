#include <engine/gpuResourceManager.hpp>
#include <graphics/tutorialParticleSystem.hpp>

using Engine::GPUResourceManager;

namespace Graphics {
    TutorialParticleSystem::TutorialParticleSystem(uint32_t width, uint32_t height, uint32_t particleCount) :
        width(width),
        height(height),
        count(particleCount) {
        std::default_random_engine rndEngine((unsigned)time(nullptr));
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

        particles.resize(particleCount);

        for (auto &particle : particles) {
            float r = 0.25f * sqrtf(rndDist(rndEngine));
            float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
            float x = r * cosf(theta) * height / width;
            float y = r * sinf(theta);

            particle.position = glm::vec2(x, y);
            particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
            particle.color    = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
        }

        allocateBuffers();
        createUniformBuffers();

        commandBuffer = CommandBuffer(Core::getInstance().getComputeCommandPool(), &Core::getInstance().getComputeQueue(), vk::CommandBufferLevel::ePrimary, Models::MAX_FRAMES_IN_FLIGHT);
    }

    TutorialParticleSystem::TutorialParticleSystem(std::nullptr_t) noexcept {}

    TutorialParticleSystem& TutorialParticleSystem::operator=(std::nullptr_t) noexcept {
        return *this;
    }

    void TutorialParticleSystem::allocateBuffers() {
       VkDeviceSize bufferSize = sizeof(Particle) * count;

       VkBufferCreateInfo createInfo = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = bufferSize,
            .usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            shaderStorageBuffers[i] = GPUResourceManager::getInstance().create(
                reinterpret_cast<std::byte*>(particles.data()), 
                createInfo,
                {},
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
    }

    void TutorialParticleSystem::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(Graphics::Models::GlobalInputs);

        VkBufferCreateInfo createInfo = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = bufferSize,
            .usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        for (size_t i = 0; i < Graphics::Models::MAX_FRAMES_IN_FLIGHT; i++) {
            uniformBuffers[i] = GPUResourceManager::getInstance().create(
                createInfo,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
    }

    void TutorialParticleSystem::updateUniformBuffer(uint32_t frameIndex) {
        Models::GlobalInputs globalInputs {
            .deltaTime = static_cast<float>(Application::getInstance().getDeltaTime())
        };

        GPUResourceManager::getInstance().update(uniformBuffers[frameIndex], &globalInputs, sizeof(globalInputs));
    }

    void TutorialParticleSystem::recordComputePass(const Pipeline &pipeline, uint32_t frameIndex) {
        commandBuffer[frameIndex].begin({});
        commandBuffer[frameIndex].bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.getInstance());
        commandBuffer[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeline.getPipelineLayout(), 0, {pipeline.getDescriptorSet(frameIndex)}, {});
        commandBuffer[frameIndex].dispatch(count / 256, 1, 1);

        commandBuffer[frameIndex].end();
    }

    void TutorialParticleSystem::executeComputePass(const Semaphore &semaphore, const uint64_t &waitValue, const uint64_t &signalValue, uint32_t frameIndex) {
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eComputeShader };

        vk::TimelineSemaphoreSubmitInfo computeTimelineInfo {
            .waitSemaphoreValueCount   = 1,
            .pWaitSemaphoreValues      = &waitValue,
            .signalSemaphoreValueCount = 1,
            .pSignalSemaphoreValues    = &signalValue
        };

        vk::SubmitInfo computeSubmitInfo {
            .pNext                = &computeTimelineInfo,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &*semaphore,
            .pWaitDstStageMask    = waitStages,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &*commandBuffer[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &*semaphore
        };

        commandBuffer.submitOnlyAsync(computeSubmitInfo, nullptr, frameIndex);
    }

    const std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT>& TutorialParticleSystem::getStorageBuffers() const { return shaderStorageBuffers; }
    const std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT>& TutorialParticleSystem::getUniformBuffers() const { return uniformBuffers; }

    const Buffer& TutorialParticleSystem::getStorageBuffer(uint32_t frameIndex) const {
        return shaderStorageBuffers[frameIndex].data();
    }

    const Buffer& TutorialParticleSystem::getUniformBuffer(uint32_t frameIndex) const {
        return uniformBuffers[frameIndex].data();
    }

    uint32_t TutorialParticleSystem::getParticleCount() const { return count; }
}