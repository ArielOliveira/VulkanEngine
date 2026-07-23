#ifndef GPU_RESOURCE_MANAGER
#define GPU_RESOURCE_MANAGER

#include <utility>

#include <vk_mem_alloc.h>

#include <vulkan/vulkan_raii.hpp>

constexpr uint32_t STAGING_BUFFER_SIZE = 16 * 1024 * 1024;


namespace Engine {
    class GPUMemoryManager {
        public:
            static GPUMemoryManager& getInstance();

            GPUMemoryManager(const GPUMemoryManager&) = delete;
            GPUMemoryManager(GPUMemoryManager&&) noexcept = delete;

            GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;
            GPUMemoryManager& operator=(GPUMemoryManager&&) noexcept = delete;

            void releaseBuffer(VkImage& image, VmaAllocation& allocation);
            void releaseBuffer(VkBuffer& buffer, VmaAllocation& allocation);

            void uploadDataToGPU(VkImage& image, VmaAllocation& allocation, const void* src, VkDeviceSize size, const VkImageCreateInfo& createInfo, vk::ImageAspectFlagBits imageAspect, uint32_t targetQueueIndex);
            void uploadDataToGPU(VkBuffer& buffer, VmaAllocation& allocation, const void* src, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t targetQueueIndex);

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
            void allocateBuffer(VkImage& image, VmaAllocation& allocation, VkDeviceSize size, const VkImageCreateInfo& createInfo, uint32_t targetQueueIndex);
            void allocateBuffer(VkBuffer& buffer, VmaAllocation& allocation, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t targetQueueIndex);

            GPUMemoryManager();

            VmaAllocator allocator;
            StagingBuffer stagingBuffer;
    };
}

#endif