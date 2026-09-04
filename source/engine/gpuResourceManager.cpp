#include <engine/gpuAllocator.hpp>
#include <vulkan/utility/vk_format_utils.h>
#include <vk_mem_alloc.h>

#include <engine/gpuResourceManager.hpp>
#include <engine/factory.hpp>

#include <graphics/core.hpp>

using Engine::Factory::ImageFTY::createImageViewInfo;
using Graphics::Core;

namespace Engine {
    GPUResourceManager& GPUResourceManager::getInstance() {
        static GPUResourceManager instance;

        return instance;
    }

    GPUResourceManager::GPUResourceManager() {
        intializeAllocator();
        createStagingBuffer();
    }

    GPUResourceManager::~GPUResourceManager() {
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
        vmaDestroyAllocator(allocator);
    }

    void GPUResourceManager::intializeAllocator() {
        VmaVulkanFunctions vulkanFunctions {
            .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr   = &vkGetDeviceProcAddr
        };

        VmaAllocatorCreateInfo allocatorCreateInfo {
            .flags             = {},
            .physicalDevice    = *Core::getInstance().getPhysicalDevice(),
            .device            = *Core::getInstance().getDevice(),
            .pVulkanFunctions  = &vulkanFunctions,
            .instance          = *Core::getInstance().getVulkanInstance(),
            .vulkanApiVersion  =  Core::getInstance().getPhysicalDevice().getProperties().apiVersion,
        };

        vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    }

    void GPUResourceManager::createStagingBuffer() {
        stagingBuffer.capacity = STAGING_BUFFER_SIZE;

        VkBufferCreateInfo bufferInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = STAGING_BUFFER_SIZE,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };

        VmaAllocationCreateInfo allocationCreateInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        vmaCreateBuffer(allocator, &bufferInfo, &allocationCreateInfo, &stagingBuffer.buffer, &stagingBuffer.allocation, &stagingBuffer.allocInfo);
    }

    void GPUResourceManager::release(const SlotKey& key, const PassKey<ResourceHandle<Buffer>>&) {
        if (--buffers.getSlot(key).refCount == 0) {
            vmaDestroyBuffer(allocator, buffers[key].buffer, bufferAllocations[key]);

            bufferStates.erase(key);
            bufferAllocations.erase(key);
            buffers.erase(key);
        }
    }

    void GPUResourceManager::release(const SlotKey& key, const PassKey<ResourceHandle<Image>>&) {
        if (--images.getSlot(key).refCount == 0) {
            vmaDestroyImage(allocator, images[key].image, imageAllocations[key]);

            imageStates.erase(key);
            imageAllocations.erase(key);
            images.erase(key);
        }
    }

    const GPUResourceManager::ImageAllocation GPUResourceManager::allocateBuffer(const VkImageCreateInfo& createInfo) {
        VmaAllocationCreateInfo allocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        VkImage           image;
        VmaAllocation     allocation;
        VmaAllocationInfo allocationInfo;

        vmaCreateImage(allocator, &createInfo, &allocationCreateInfo, &image, &allocation, &allocationInfo);

        return { std::move(image), std::move(allocation), std::move(allocationInfo) };
    }

    const GPUResourceManager::BufferAllocation GPUResourceManager::allocateBuffer(const VkBufferCreateInfo& createInfo) {
        VmaAllocationCreateInfo allocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        
        VkBuffer      buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;

        vmaCreateBuffer(allocator, &createInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo);

        return { std::move(buffer), std::move(allocation), std::move(allocationInfo) };
    }

    const ResourceHandle<Image> GPUResourceManager::create(const VkImageCreateInfo& createInfo) {
        auto [image, allocation, allocationInfo] = allocateBuffer(createInfo);

        /*return {
            make_state_array({}),
            VK_QUEUE_FAMILY_IGNORED
        };*/
    }

    const ResourceHandle<Image> GPUResourceManager::create(const std::byte* src, const VkDeviceSize size, const ImageLayout& imageLayout, const VkImageCreateInfo& createInfo, const VkImageAspectFlags imageAspect) {
        auto [image, allocation, allocationInfo] = allocateBuffer(createInfo);

        vk::ImageMemoryBarrier2 barrier {
            .srcStageMask           = vk::PipelineStageFlagBits2::eTransfer,    .srcAccessMask          = vk::AccessFlagBits2::eNone,
            .dstStageMask           = vk::PipelineStageFlagBits2::eTransfer,    .dstAccessMask          = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout              = vk::ImageLayout::eUndefined,              .newLayout              = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex    = vk::QueueFamilyIgnored,                   .dstQueueFamilyIndex    = vk::QueueFamilyIgnored,
            .image                  = image,
            .subresourceRange       = { .aspectMask = static_cast<vk::ImageAspectFlags>(imageAspect), .baseMipLevel = 0, .levelCount = static_cast<uint32_t>(imageLayout.mips.size()), .baseArrayLayer = 0, .layerCount = imageLayout.layers }
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };

        CommandBuffer::singleTimeTransfer().addBarrier(dependencyInfo).submit();

        vk::BufferImageCopy2 region {
            .bufferOffset       = 0,
            .bufferRowLength    = 0,
            .bufferImageHeight  = 0,
            .imageSubresource   = { .aspectMask = static_cast<vk::ImageAspectFlags>(imageAspect), .mipLevel = 0, .baseArrayLayer = 0, .layerCount = imageLayout.layers},
            .imageOffset        = { 0, 0, 0 },
            .imageExtent        = { imageLayout.mips[0].width, imageLayout.mips[0].height, imageLayout.mips[0].depth }
        };

        vk::CopyBufferToImageInfo2 copyInfo {
            .srcBuffer       = stagingBuffer.buffer,
            .dstImage        = image,
            .dstImageLayout  = vk::ImageLayout::eTransferDstOptimal,
            .regionCount     = 1,
            .pRegions        = &region
        };

        for (size_t i = 0; i < imageLayout.mips.size(); i++) {
            auto mipLayout = imageLayout.mips[i];

            region.imageSubresource.mipLevel = i;
            
            size_t storageRows   = (mipLayout.height + imageLayout.blockHeight - 1) / imageLayout.blockHeight;
            
            size_t rowsPerChunk  = std::max(1ULL, (static_cast<size_t>(stagingBuffer.capacity) / mipLayout.storageRowPitch));
            size_t rowsWritten   = 0;
            
            while(rowsWritten < storageRows) {
                size_t rowsToWrite = std::min(rowsPerChunk, storageRows - rowsWritten); 
                size_t bytes       = (size_t)rowsToWrite * (size_t)mipLayout.storageRowPitch;   

                memcpy(stagingBuffer.allocInfo.pMappedData, (src + mipLayout.offset) + (bytes * rowsWritten), bytes);

                uint32_t currentY      = rowsWritten * imageLayout.blockHeight;
                uint32_t rowsThisChunk = std::min(static_cast<uint32_t>(rowsToWrite) * imageLayout.blockHeight, mipLayout.height - currentY);

                region.imageOffset.y = currentY;
                region.setImageExtent({ mipLayout.width, rowsThisChunk, 1 });

                CommandBuffer::singleTimeTransfer().copyBufferToImage(copyInfo).submit();

                rowsWritten += rowsToWrite;
            }
        }

        /*return {
            make_state_array({ vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal }),
            createInfo.sharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT ? 
                 VK_QUEUE_FAMILY_IGNORED :
                 Core::getInstance().getTransferQueueIndex() 
        };*/
    }

    const ResourceHandle<Buffer> GPUResourceManager::create(const std::byte* src, const VkBufferCreateInfo& createInfo) {
        auto [buffer, allocation, allocationInfo] = allocateBuffer(createInfo);

        VkDeviceSize written = 0;
        
        while (written < createInfo.size) {
            VkDeviceSize chunkSize = std::min(stagingBuffer.capacity, createInfo.size - written);
            
            std::cout << "Writting GPU memory: " << written << "/" << createInfo.size << '\n';

            memcpy(stagingBuffer.allocInfo.pMappedData, src + written, (size_t)chunkSize);

            CommandBuffer::singleTimeTransfer().copyBuffer(stagingBuffer.buffer, buffer, vk::BufferCopy(0, written, chunkSize)).submit();

            written += chunkSize;
        }

        /*return { vk::PipelineStageFlagBits2::eTransfer, 
                 vk::AccessFlagBits2::eTransferWrite,
                 createInfo.sharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT ? 
                 VK_QUEUE_FAMILY_IGNORED :
                 Core::getInstance().getTransferQueueIndex() };*/
    }

    const ResourceHandle<Image>  GPUResourceManager::registerExternal(const VkImage& image, const VkImageAspectFlags aspectMask, const VkFormat format,
                                                          const uint32_t width, const uint32_t height, const uint32_t depth,
                                                          const uint32_t mipCount = 1, const uint32_t layerCount = 1,
                                                          const VmaAllocation& allocation = VK_NULL_HANDLE) {
        VkImageViewType viewType = depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : 
                           std::min(height, width) > 1 ? 
                           VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
            
        VkImageViewCreateInfo viewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image,
            .viewType         = viewType,
            .format           = format,
            .subresourceRange = { .aspectMask = aspectMask, .baseMipLevel = 0, .levelCount = mipCount, .baseArrayLayer = 0, .layerCount = layerCount }
        };

        VkImageView view;
        vkCreateImageView(*Core::getInstance().getDevice(), &viewCreateInfo, nullptr, &view);

        const SlotKey key = images.emplace(
            std::move(image), 
            std::move(view),
            std::move(viewCreateInfo.format),
            std::move(viewCreateInfo.subresourceRange.aspectMask),
            std::move(width), std::move(height), std::move(depth),
            std::move(mipCount), std::move(layerCount),
            std::move(vkuFormatComponentCount(format))
        );

        imageStates.emplace(key, make_state_array({}));

        return ResourceHandle<Image>(key);
    }

    void GPUResourceManager::updateState(const SlotKey& key, const ImageState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx) {
        assert(images.contains(key));

        vk::ImageMemoryBarrier2 barrier {
            .image                  = images[key].image,
            .subresourceRange       = { .aspectMask = images[key].aspect, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };

        for (size_t i = 0; i < images[key].mipCount; i++) {
            auto oldS         = imageStates[key].states[i];
            auto newS         = newState.states[i];

            barrier.srcStageMask  = oldS.stage;
            barrier.dstStageMask  = newS.stage;

            barrier.srcAccessMask = oldS.access;
            barrier.dstAccessMask = newS.access;

            barrier.oldLayout     = oldS.layout;
            barrier.newLayout     = newS.layout;

            barrier.subresourceRange.baseMipLevel = i;

            commandBuffer.addBarrier(dependencyInfo, cbIdx);
        }

        imageStates[key] = std::move(newState); 
    }
    
    void GPUResourceManager::updateState(const SlotKey& key, const VkDeviceSize size, const VkDeviceSize offset, const BufferState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx) {
        vk::BufferMemoryBarrier2 barrier {
            .srcStageMask = bufferStates[key].currentStage, .srcAccessMask = bufferStates[key].currentAccess,
            .dstStageMask = newState.currentStage,          .dstAccessMask = newState.currentAccess,
            .buffer = buffers[key].buffer, .offset = offset, .size = size
        };

        vk::DependencyInfo dependencyInfo {
            .dependencyFlags          = {},
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers    = &barrier
        };

        commandBuffer.addBarrier(dependencyInfo, cbIdx);
    }
    
    void GPUResourceManager::transferQueueFamily(const SlotKey& key, const ImageState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx, const uint32_t dstIdx) {
        assert(images.contains(key));
        assert(imageStates[key].currentQueueFamily != VK_QUEUE_FAMILY_IGNORED &&
               newState.currentQueueFamily         != VK_QUEUE_FAMILY_IGNORED &&
               imageStates[key].currentQueueFamily != newState.currentQueueFamily);

        vk::ImageMemoryBarrier2 barrier {
            .image                  = images[key].image,
            .subresourceRange       = { .aspectMask = static_cast<vk::ImageAspectFlags>(images[key].aspect), .baseMipLevel = 0, .levelCount = images[key].mipCount, .baseArrayLayer = 0, .layerCount = images[key].layers }
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };

        if (imageStates[key].currentQueueFamily != newState.currentQueueFamily) {
            barrier.subresourceRange.levelCount = images[key].mipCount;
            barrier.subresourceRange.layerCount = images[key].layers;

            barrier.srcStageMask  = imageStates[key].states[0].stage;
            barrier.dstStageMask  = imageStates[key].states[0].stage;

            barrier.srcAccessMask = imageStates[key].states[0].access;
            barrier.dstAccessMask = imageStates[key].states[0].access;

            src.addBarrier(dependencyInfo, srcIdx).submit(srcIdx);
        }

        dst.addBarrier(dependencyInfo, dstIdx).submit(dstIdx);
    }
    
    void GPUResourceManager::transferQueueFamily(const SlotKey& key, const VkDeviceSize size, const VkDeviceSize offset, const BufferState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx, const uint32_t dstIdx) {
        assert(buffers.contains(key));
        assert(bufferStates[key].currentQueueFamily != VK_QUEUE_FAMILY_IGNORED &&
               newState.currentQueueFamily          != VK_QUEUE_FAMILY_IGNORED &&
               bufferStates[key].currentQueueFamily != newState.currentQueueFamily);

        vk::BufferMemoryBarrier2 barrier {
            .srcStageMask = bufferStates[key].currentStage, .srcAccessMask = bufferStates[key].currentAccess,
            .dstStageMask = bufferStates[key].currentStage, .dstAccessMask = bufferStates[key].currentAccess,
            .srcQueueFamilyIndex = bufferStates[key].currentQueueFamily, .dstQueueFamilyIndex = newState.currentQueueFamily,
            .buffer = buffers[key].buffer, .offset = offset, .size = size
        };

        vk::DependencyInfo dependencyInfo {
            .dependencyFlags          = {},
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers    = &barrier
        };

        src.addBarrier(dependencyInfo, srcIdx).submit(srcIdx);
        dst.addBarrier(dependencyInfo, dstIdx).submit(dstIdx);
    }
}