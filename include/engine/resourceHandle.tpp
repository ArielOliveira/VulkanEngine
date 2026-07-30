#ifndef ENGINE_RESOURCE_HANDLE_TPP
#define ENGINE_RESOURCE_HANDLE_TPP

#include <engine/resourceHandle.hpp>

namespace Engine {
    template <typename T>
    ResourceHandle<T>::ResourceHandle(std::nullptr_t) noexcept : _key({~0U, ~0U}) {}

    template <typename T>
    ResourceHandle<T>::ResourceHandle(SlotKey key) noexcept : _key(key) {
        ResourceManager::getInstance().acquire<T>(key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(const ResourceHandle& rhs) noexcept : _key(rhs._key) {
        ResourceManager::getInstance().acquire<T>(_key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(ResourceHandle&& rhs) noexcept { std::swap(_key, rhs._key); }

    template <typename T>
    ResourceHandle<T>::~ResourceHandle() noexcept {
        if (_key.index != ~0U)
        ResourceManager::getInstance().release<T>(_key, {});
    }

    template <typename T>
    const ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t) noexcept {
        _key = { ~0U, ~0U };
    }

    template <typename T>
    ResourceHandle<T>& ResourceHandle<T>::operator=(ResourceHandle&& rhs) noexcept {
        if (this != &rhs) 
            std::swap(_key, rhs._key);
        
        rhs._key = { ~0U, ~0U };

        return *this;
    }

    template <typename T>
    const SlotKey& ResourceHandle<T>::key() const noexcept {
        return _key;
    }

    template <typename T>
    const T& ResourceHandle<T>::data() const {
        return ResourceManager::getInstance().get<T>(_key, {});
    }
}

#endif