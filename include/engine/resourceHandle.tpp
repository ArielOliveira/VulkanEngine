#ifndef ENGINE_RESOURCE_HANDLE_TPP
#define ENGINE_RESOURCE_HANDLE_TPP

#include <engine/resourceHandle.hpp>

namespace Engine {
    template <typename T>
    ResourceHandle<T>::ResourceHandle(std::nullptr_t) noexcept : key({~0U, ~0U}) {}

    template <typename T>
    ResourceHandle<T>::ResourceHandle(SlotKey key) noexcept : key(key) {
        ResourceManager::getInstance().acquire<T>(key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(const ResourceHandle& rhs) noexcept : key(rhs.key) {
        ResourceManager::getInstance().acquire<T>(key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(ResourceHandle&& rhs) noexcept { std::swap(key, rhs.key); }

    template <typename T>
    ResourceHandle<T>::~ResourceHandle() noexcept {
        if (key.index != ~0U)
        ResourceManager::getInstance().release<T>(key, {});
    }

    template <typename T>
    const ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t) noexcept {
        key = { ~0U, ~0U };
    }

    template <typename T>
    ResourceHandle<T>& ResourceHandle<T>::operator=(ResourceHandle&& rhs) noexcept {
        if (this != &rhs) 
            std::swap(key, rhs.key);
        
        rhs.key = { ~0U, ~0U };

        return *this;
    }

    template <typename T>
    const T& ResourceHandle<T>::data() const {
        return ResourceManager::getInstance().get<T>(key, {});
    }
}

#endif