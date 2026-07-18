#ifndef GPU_RESOURCE_MANAGER
#define GPU_RESOURCE_MANAGER

#include <utility>

#include <vk_mem_alloc.h>


constexpr uint32_t STAGING_BUFFER_SIZE = 16 * 1024 * 1024;

namespace Engine {
    class GPUMemoryManager {
        public:
            static GPUMemoryManager& getInstance();

            GPUMemoryManager(const GPUMemoryManager&) = delete;
            GPUMemoryManager(GPUMemoryManager&&) noexcept = delete;

            GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;
            GPUMemoryManager& operator=(GPUMemoryManager&&) noexcept = delete;

            void releaseBuffer(VkBuffer& buffer, VmaAllocation& allocation);
            void uploadDataToGPU(VkBuffer& buffer, VmaAllocation& allocation, const void* src, VkDeviceSize size, VkBufferUsageFlags usage);

            ~GPUMemoryManager();

        private:
            struct StagingBuffer {
                VkBuffer buffer;
                VmaAllocation allocation;
                VmaAllocationInfo allocInfo;
                VkDeviceSize capacity;
            };

            void intializeAllocator();
            void createStagingBuffer();
            void allocateBuffer(VkBuffer& buffer, VmaAllocation& allocation, VkDeviceSize size, VkBufferUsageFlags usage);

            GPUMemoryManager();

            VmaAllocator allocator;
            StagingBuffer stagingBuffer;
    };
}

#endif