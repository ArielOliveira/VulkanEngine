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
}