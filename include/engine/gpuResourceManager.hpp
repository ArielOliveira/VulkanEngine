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

using Utils::SlotMap;
using Utils::SparseSet;
using Utils::PassKey;

using namespace Engine::Resources::GPU;
using namespace Engine::Resources::Trackers;

using Engine::Resources::Pools;
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

            template <typename Resource>
            void release(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);

            template <typename Resource>
            void acquire(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);

            template <typename Resource>
            const Resource& get(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const;

            template <typename Resource>
            const ResourceState& getState(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const;

            template <typename Resource>
            const bool contains(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const;
            
            ResourceHandle<Image>  create(const VkImageCreateInfo& createInfo, const VkImageAspectFlags aspectMask);
            ResourceHandle<Image>  create(const std::byte* src, const VkDeviceSize size, const ImageLayout& imageLayout,  const VkImageCreateInfo& createInfo, const VkImageAspectFlags aspectMask);
            ResourceHandle<Buffer> create(const std::byte* src, const VkBufferCreateInfo& createInfo);

            ResourceHandle<Image>  registerExternal(const VkImage& image, const VkImageAspectFlags imageAspect, const VkFormat format,
                                                          const uint32_t width, const uint32_t height, const uint32_t depth,
                                                          const uint32_t mipCount = 1, const uint32_t layerCount = 1);

            void transferQueueFamily(const ResourceHandle<Image>& key, const ImageState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx = 0, const uint32_t dstIdx = 0);
            void transferQueueFamily(const ResourceHandle<Buffer>& key, const VkDeviceSize size, const VkDeviceSize offset, const BufferState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx = 0, const uint32_t dstIdx = 0);

            void clearLayout(const ResourceHandle<Image>& key);
            
            template <typename Resource>
            void clearAccess(const ResourceHandle<Resource>& key);

            template <typename Resource>
            void clearStage(const ResourceHandle<Resource>& key);

            template <typename Resource>
            void clearState(const ResourceHandle<Resource>& key);
    
            void updateState(const ResourceHandle<Image>& key, const ImageState& newState);
            void updateState(const ResourceHandle<Buffer>& key, const VkDeviceSize size, const VkDeviceSize offset, const BufferState& newState);
            
            void setBarrier(const ResourceHandle<Image>& key, const ImageState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx = 0);
            void setBarrier(const ResourceHandle<Buffer>& key, const VkDeviceSize size, const VkDeviceSize offset, const BufferState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx = 0);

            ~GPUResourceManager();

        private:
            using ResourcesPool = Pools<Buffer, Image>;

            using ImageAllocation  = std::tuple<VkImage, VmaAllocation, VmaAllocationInfo>;
            using BufferAllocation = std::tuple<VkBuffer, VmaAllocation, VmaAllocationInfo>;

            struct StagingBuffer {
                VkBuffer buffer;
                VmaAllocation allocation;
                VmaAllocationInfo allocInfo;
                VkDeviceSize capacity;
            };

            GPUResourceManager();

            void intializeAllocator();
            void createStagingBuffer();

            const ImageAllocation   allocateBuffer(const VkImageCreateInfo& createInfo);
            const BufferAllocation  allocateBuffer(const VkBufferCreateInfo& createInfo);

            VmaAllocator  allocator;
            StagingBuffer stagingBuffer;

            ResourcesPool                   pool;
            
            SparseSet<BufferState>          bufferStates;
            SparseSet<ImageState>           imageStates;

            SparseSet<VmaAllocation>        bufferAllocations;
            SparseSet<VmaAllocation>        imageAllocations;
    };
}

#include <engine/gpuResourceManager.tpp>

#endif