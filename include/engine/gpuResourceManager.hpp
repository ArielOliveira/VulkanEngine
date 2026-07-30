#ifndef GPU_RESOURCE_MANAGER
#define GPU_RESOURCE_MANAGER

#include <utility>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

#include <utils/passKey.hpp>
#include <utils/sparseset.hpp>

#include <engine/resources.hpp>

using Utils::SparseSet;
using Engine::Resources::ImageState;

constexpr uint32_t STAGING_BUFFER_SIZE = 16 * 1024 * 1024;

namespace Engine {
    class GPUResourceManager {
        public:
            static GPUResourceManager& getInstance();

            GPUResourceManager(const GPUResourceManager&) = delete;
            GPUResourceManager(GPUResourceManager&&) noexcept = delete;

            GPUResourceManager& operator=(const GPUResourceManager&) = delete;
            GPUResourceManager& operator=(GPUResourceManager&&) noexcept = delete;

            void releaseBuffer(VkImage& image, VmaAllocation& allocation);
            void releaseBuffer(VkBuffer& buffer, VmaAllocation& allocation);

            void uploadData(VkImage& image, VmaAllocation& allocation, const void* src, const VkDeviceSize size, const VkImageCreateInfo& createInfo, const vk::ImageAspectFlagBits imageAspect, const uint32_t targetQueueIndex);
            void uploadData(VkBuffer& buffer, VmaAllocation& allocation, const void* src, const VkDeviceSize size, const VkBufferUsageFlags usage, const uint32_t targetQueueIndex);

            void updateResourceState(const SlotKey& key, const ImageState& state);
            void registerResourceState(const SlotKey& key, const ImageState& initialState);
            void unregisterResourceState(const SlotKey& key);

            ~GPUResourceManager();

        private:
            struct StagingBuffer {
                VkBuffer buffer;
                VmaAllocation allocation;
                VmaAllocationInfo allocInfo;
                VkDeviceSize capacity;
            };

            void intializeAllocator();
            void createStagingBuffer();
            void allocateBuffer(VkImage& image, VmaAllocation& allocation, const VkDeviceSize size, const VkImageCreateInfo& createInfo, const uint32_t targetQueueIndex);
            void allocateBuffer(VkBuffer& buffer, VmaAllocation& allocation, const VkDeviceSize size, const VkBufferUsageFlags usage, const uint32_t targetQueueIndex);

            GPUResourceManager();

            VmaAllocator allocator;
            StagingBuffer stagingBuffer;

            SparseSet<ImageState> imageStates;
    };
}

#endif