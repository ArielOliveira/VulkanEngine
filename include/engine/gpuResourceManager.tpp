#ifndef ENGINE_GPU_RESOURCE_MANAGER_TPP
#define ENGINE_GPU_RESOURCE_MANAGER_TPP

#include <engine/gpuResourceManager.hpp>
#include <graphics/core.hpp>

using Graphics::Core;

namespace Engine {
    template <>
    inline void GPUResourceManager::release<Buffer>(const SlotKey& key, const PassKey<ResourceHandle<Buffer>>&) {
        if (--pool.get<Buffer>().getSlot(key).refCount == 0) {
            pool.get<Buffer>().getSlot(key).state = ResourceState::Releasing;
            
            vmaDestroyBuffer(allocator, pool.get<Buffer>()[key].buffer, bufferAllocations[key]);

            bufferStates.erase(key);
            bufferAllocations.erase(key);
            pool.get<Buffer>().erase(key);
        }
    }

    template <>
    inline void GPUResourceManager::release<Image>(const SlotKey& key, const PassKey<ResourceHandle<Image>>&) {
        if (--pool.get<Image>().getSlot(key).refCount == 0) {
            pool.get<Image>().getSlot(key).state = ResourceState::Releasing;

            vkDestroyImageView(*Core::getInstance().getDevice(), pool.get<Image>()[key].view, nullptr);
            
            if (imageAllocations.contains(key)) {
                vmaDestroyImage(allocator, pool.get<Image>()[key].image, imageAllocations[key]);
                imageAllocations.erase(key);
            }

            imageStates.erase(key);
            pool.get<Image>().erase(key);
        }
    }

    template <typename Resource>
    inline void GPUResourceManager::acquire(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) {
        assert(pool.get<Resource>().getSlot(key).generations == key.generations);

        pool.get<Resource>().getSlot(key).refCount++;
    }

    template <typename Resource>
    inline const Resource& GPUResourceManager::get(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const {
        assert(pool.get<Resource>().getSlot(key).generations == key.generations);

        return pool.get<Resource>()[key];
    }

    template <typename Resource>
    inline const ResourceState& GPUResourceManager::getState(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const {
        assert(pool.get<Resource>().getSlot(key).generations == key.generations);

        return pool.get<Resource>().getSlot(key).state;
    }

    template <typename Resource>
    inline const bool GPUResourceManager::contains(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const {
        return pool.get<Resource>().contains(key);
    }
}

#endif