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
using namespace Engine::Resources::Trackers;
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
            void transferResourceQueueFamily(const ResourceHandle<ResourceType>& handle, const ResourceState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx = 0, const uint32_t dstIdx = 0);
            
            template<typename ResourceType, typename ResourceState>
            void updateResourceState(const ResourceHandle<ResourceType>& handle, const ResourceState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx = 0);

            template<typename ResourceType, typename ResourceState>
            void updateResourceState(const ResourceHandle<ResourceType>& handle, const ResourceState& newState);
            
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
            void updateImageState(const SlotKey& key, const Image&  image,  const ImageState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx = 0);
            void updateBufferState(const SlotKey& key, const VkBuffer& buffer, const VkDeviceSize size, const VkDeviceSize offset, const BufferState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx = 0);

            void transferImageQueueFamily(const SlotKey& key, const Image& image, const ImageState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx = 0, const uint32_t dstIdx = 0);
            void transferBufferQueueFamily(const SlotKey& key, const VkBuffer& buffer, const VkDeviceSize size, const VkDeviceSize offset, const BufferState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx = 0, const uint32_t dstIdx = 0);

            GPUResourceManager();

            VmaAllocator allocator;
            StagingBuffer stagingBuffer;

            SparseSet<ImageState>  imageStates;
            SparseSet<BufferState> bufferStates;
            
    };
}

#include <engine/gpuResourceManager.tpp>

#endif