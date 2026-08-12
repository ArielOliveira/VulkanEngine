#ifndef ENGINE_GPU_RESOURCE_MANAGER_TPP
#define ENGINE_GPU_RESOURCE_MANAGER_TPP

#include <engine/gpuResourceManager.hpp>

namespace Engine {
    template<>
    inline void GPUResourceManager::updateResourceState<Image, ImageState>(const ResourceHandle<Image>& handle, const ImageState& newState, const CommandBuffer& commandBuffer) { 
        assert(imageStates.contains(handle.key()));

        vk::ImageMemoryBarrier2 barrier {
            .image                  = handle.data().image,
            .subresourceRange       = { .aspectMask = static_cast<vk::ImageAspectFlags>(handle.data().aspectFlags), .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };

        if (imageStates[handle.key()].currentQueueFamily != newState.currentQueueFamily) {
            barrier.subresourceRange.levelCount = handle.data().mipCount;
            barrier.subresourceRange.layerCount = handle.data().layers;

            barrier.srcStageMask  = imageStates[handle.key()].currentStage[0];
            barrier.dstStageMask  = imageStates[handle.key()].currentStage[0];

            barrier.srcAccessMask = imageStates[handle.key()].currentAccess[0];
            barrier.dstAccessMask = imageStates[handle.key()].currentAccess[0];

            
        }

        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;

        for (size_t i = 0; i < handle.data().mipCount; i++) {
            barrier.srcStageMask  = imageStates[handle.key()].currentStage[i];
            barrier.dstStageMask  = newState.currentStage[i];

            barrier.srcAccessMask = imageStates[handle.key()].currentAccess[i];
            barrier.dstAccessMask = newState.currentAccess[i];
        }
        
        imageStates[handle.key()] = std::move(newState); 
    }

    template<>
    inline void GPUResourceManager::updateResourceState<Mesh, BufferState>(const ResourceHandle<Mesh>& handle, const BufferState& newState, const CommandBuffer& commandBuffer) { 
        assert(bufferStates.contains(handle.key()));

        bufferStates[handle.key()] = std::move(newState); 
    }

    template<>
    inline void GPUResourceManager::registerResourceState<ImageState>(const SlotKey& key, const ImageState& initialState) { imageStates.emplace(key, std::move(initialState)); }

    template<>
    inline void GPUResourceManager::unregisterResourceState<ImageState>(const SlotKey& key) { imageStates.erase(key); }
    
    template<>
    inline void GPUResourceManager::registerResourceState<BufferState>(const SlotKey& key, const BufferState& initialState) { bufferStates.emplace(key, std::move(initialState)); }
    
    template<>
    inline void GPUResourceManager::unregisterResourceState<BufferState>(const SlotKey& key) { bufferStates.erase(key); }
}

#endif