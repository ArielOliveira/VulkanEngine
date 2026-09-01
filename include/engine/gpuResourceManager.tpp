#ifndef ENGINE_GPU_RESOURCE_MANAGER_TPP
#define ENGINE_GPU_RESOURCE_MANAGER_TPP

#include <engine/gpuResourceManager.hpp>

namespace Engine {
    template<>
    inline void GPUResourceManager::transferResourceQueueFamily<Mesh, BufferState>(const ResourceHandle<Mesh>& handle, const BufferState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx, const uint32_t dstIdx) {
        assert(bufferStates.contains(handle.key()));
        assert(bufferStates[handle.key()].currentQueueFamily != VK_QUEUE_FAMILY_IGNORED &&
               newState.currentQueueFamily                   != VK_QUEUE_FAMILY_IGNORED &&
               bufferStates[handle.key()].currentQueueFamily != newState.currentQueueFamily);

        transferBufferQueueFamily(handle.key(), handle.data().vertexBuffer, handle.data().vtxBufferSize, 0, newState, src, dst, srcIdx, dstIdx);
        if (handle.data().indexCount > 0)
            transferBufferQueueFamily(handle.key(), handle.data().indexBuffer, handle.data().idxBufferSize, 0, newState, src, dst, srcIdx, dstIdx);
    }

    template<>
    inline void GPUResourceManager::transferResourceQueueFamily<Image, ImageState>(const ResourceHandle<Image>& handle, const ImageState& newState, const CommandBuffer& src, const CommandBuffer& dst, const uint32_t srcIdx, const uint32_t dstIdx) {
        assert(imageStates.contains(handle.key()));
        assert(imageStates[handle.key()].currentQueueFamily != VK_QUEUE_FAMILY_IGNORED &&
               newState.currentQueueFamily                  != VK_QUEUE_FAMILY_IGNORED &&
               imageStates[handle.key()].currentQueueFamily != newState.currentQueueFamily);

        transferImageQueueFamily(handle.key(), handle.data(), newState, src, dst, srcIdx, dstIdx);
    }

    template<>
    inline void GPUResourceManager::updateResourceState<Image, ImageState>(const ResourceHandle<Image>& handle, const ImageState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx) { 
        assert(imageStates.contains(handle.key()));

        updateImageState(handle.key(), handle.data(), newState, commandBuffer, cbIdx);
    }

    template<>
    inline void GPUResourceManager::updateResourceState<Mesh, BufferState>(const ResourceHandle<Mesh>& handle, const BufferState& newState, const CommandBuffer& commandBuffer, const uint32_t cbIdx) { 
        assert(bufferStates.contains(handle.key()));

        updateBufferState(handle.key(), handle.data().vertexBuffer, handle.data().vtxBufferSize, 0, newState, commandBuffer, cbIdx);
        if (handle.data().indexCount > 0)
            updateBufferState(handle.key(), handle.data().indexBuffer, handle.data().idxBufferSize, 0, newState, commandBuffer, cbIdx);
    }

    template<>
    inline void GPUResourceManager::updateResourceState<Image, ImageState>(const ResourceHandle<Image>& handle, const ImageState& newState) {
        assert(imageStates.contains(handle.key()));

        imageStates[handle.key()] = std::move(newState);
    }

    template<>
    inline void GPUResourceManager::updateResourceState<Mesh, BufferState>(const ResourceHandle<Mesh>& handle, const BufferState& newState) {
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