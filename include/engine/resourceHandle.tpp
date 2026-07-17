#ifndef ENGINE_RESOURCE_HANDLE_TPP
#define ENGINE_RESOURCE_HANDLE_TPP

#include <utils/slotmap.hpp>
using Utils::SlotKey;

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
    ResourceHandle<T>::~ResourceHandle() noexcept {
        ResourceManager::getInstance().release<T>(key, {});
    }

    template <typename T>
    const ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t) noexcept {}
}

#endif
