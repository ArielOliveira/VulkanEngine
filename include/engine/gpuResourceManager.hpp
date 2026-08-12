#ifndef ENGINE_GPU_RESOURCE_MANAGER_HPP
#define ENGINE_GPU_RESOURCE_MANAGER_HPP

#include <utility>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

#include <utils/passKey.hpp>
#include <utils/sparseset.hpp>

#include <engine/resources.hpp>
#include <engine/descriptors.hpp>
#include <graphics/commandBuffer.hpp>

using Utils::SparseSet;
using namespace Engine::Resources;
using Engine::Descriptors::ImageLayout;
using Engine::Descriptors::ImageMipLayout;

using Graphics::CommandBuffer;

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
            const ImageState  uploadData(VkImage& image, VmaAllocation& allocation, const VkImageCreateInfo& createInfo);
            const ImageState  uploadData(VkImage& image, VmaAllocation& allocation, const std::byte* src, const VkDeviceSize size, const ImageLayout& imageLayout,  const VkImageCreateInfo& createInfo, const VkImageAspectFlags imageAspect);
            const BufferState uploadData(VkBuffer& buffer, VmaAllocation& allocation, const std::byte* src, const VkBufferCreateInfo& createInfo);

            
            template<typename ResourceType, typename ResourceState>
            void updateResourceState(const ResourceHandle<ResourceType>& handle, const ResourceState& newState, const CommandBuffer& commandBuffer);
            
            template<typename ResourceState>
            void registerResourceState(const SlotKey& key, const ResourceState& newState);

            template<typename ResourceState>
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
            void allocateBuffer(VkImage& image, VmaAllocation& allocation, const VkImageCreateInfo& createInfo);
            void allocateBuffer(VkBuffer& buffer, VmaAllocation& allocation, const VkBufferCreateInfo& createInfo);

            GPUResourceManager();

            VmaAllocator allocator;
            StagingBuffer stagingBuffer;

            SparseSet<ImageState>  imageStates;
            SparseSet<BufferState> bufferStates;
            
    };
}

#include <engine/gpuResourceManager.tpp>

#endif