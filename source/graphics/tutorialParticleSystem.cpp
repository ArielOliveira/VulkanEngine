#include <graphics/tutorialParticleSystem.hpp>

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
        std::array<uint32_t, 2> queueFamilyIndices = { Core::getInstance().getComputeQueueIndex(), Core::getInstance().getTransferQueueIndex() };
        vk::DeviceSize bufferSize = sizeof(Particle) * count;

        auto [stagingBuffer, stagingBufferMemory] = Core::getInstance().createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::SharingMode::eExclusive,
            Core::getInstance().getTransferQueueIndex(),
            nullptr
        );

        void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(dataStaging, particles.data(), (size_t)bufferSize);
        stagingBufferMemory.unmapMemory();

        for (size_t i = 0; i < Models::MAX_FRAMES_IN_FLIGHT; i++) {
            auto [buffer, bufferMem] = Core::getInstance().createBuffer(
                bufferSize,
                vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                Core::getInstance().hasDedicatedTransferQueue() ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                Core::getInstance().getTransferQueueIndex(),
                Core::getInstance().hasDedicatedTransferQueue() ? queueFamilyIndices.data()    : nullptr
            );

            CommandBuffer::singleTimeTransfer().copyBuffer(stagingBuffer, buffer, bufferSize).submit();

            shaderStorageBuffers.emplace_back(std::move(buffer));
            shaderStorageBuffersMemory.emplace_back(std::move(bufferMem));
        }
    }

    void TutorialParticleSystem::createUniformBuffers() {
        std::array<uint32_t, 2> queueFamilyIndices = { Core::getInstance().getGraphicsQueueIndex(), Core::getInstance().getTransferQueueIndex() };

        for (size_t i = 0; i < Graphics::Models::MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DeviceSize bufferSize = sizeof(Graphics::Models::GlobalInputs);
            auto [buffer, bufferMem] = Core::getInstance().createBuffer(
                bufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                Core::getInstance().hasDedicatedTransferQueue() ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                Core::getInstance().getTransferQueueIndex(),
                Core::getInstance().hasDedicatedTransferQueue() ? queueFamilyIndices.data()    : nullptr
            );

            uniformBuffers.emplace_back(std::move(buffer));
            uniformBuffersMemory.emplace_back(std::move(bufferMem));
            uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));
        }
    }

    void TutorialParticleSystem::updateUniformBuffer(uint32_t frameIndex) {
        Models::GlobalInputs globalInputs {
            .deltaTime = static_cast<float>(Application::getInstance().getDeltaTime())
        };

        memcpy(uniformBuffersMapped[frameIndex], &globalInputs, sizeof(Models::GlobalInputs));
    }

    void TutorialParticleSystem::executeComputePass(const Pipeline &pipeline, const Semaphore &semaphore, const uint64_t &waitValue, const uint64_t &signalValue, uint32_t frameIndex) {
        commandBuffer[frameIndex].reset();
        commandBuffer[frameIndex].begin({});
        commandBuffer[frameIndex].bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.getInstance());
        commandBuffer[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeline.getPipelineLayout(), 0, {pipeline.getDescriptorSet(frameIndex)}, {});
        commandBuffer[frameIndex].dispatch(count / 256, 1, 1);

        vk::TimelineSemaphoreSubmitInfo computeTimelineInfo {
            .waitSemaphoreValueCount   = 1,
            .pWaitSemaphoreValues      = &waitValue,
            .signalSemaphoreValueCount = 1,
            .pSignalSemaphoreValues    = &signalValue
        };

        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eComputeShader };

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

        commandBuffer.submitAsync(computeSubmitInfo, nullptr, frameIndex);
    }

    const vector<vk::raii::Buffer>& TutorialParticleSystem::getStorageBuffers() const { return shaderStorageBuffers; }
    const vector<vk::raii::Buffer>& TutorialParticleSystem::getUniformBuffers() const { return uniformBuffers; }

    const vk::raii::Buffer& TutorialParticleSystem::getStorageBuffer(uint32_t frameIndex) const {
        return shaderStorageBuffers[frameIndex];
    }

    const vk::raii::Buffer& TutorialParticleSystem::getUniformBuffer(uint32_t frameIndex) const {
        return uniformBuffers[frameIndex];
    }

    uint32_t TutorialParticleSystem::getParticleCount() const { return count; }
}