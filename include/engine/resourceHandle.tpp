#ifndef ENGINE_RESOURCE_HANDLE_TPP
#define ENGINE_RESOURCE_HANDLE_TPP

#include <engine/resourceHandle.hpp>

namespace Engine {
    template <typename T>
    ResourceHandle<T>::ResourceHandle(std::nullptr_t) noexcept : m_key(null_key) {}

    template <typename T>
    ResourceHandle<T>::ResourceHandle(SlotKey key) noexcept : m_key(key) {
        ResourceManager::getInstance().acquire<T>(key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(const ResourceHandle& rhs) noexcept : m_key(rhs.m_key) {
        ResourceManager::getInstance().acquire<T>(m_key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(ResourceHandle&& rhs) noexcept { std::swap(m_key, rhs.m_key); }

    template <typename T>
    ResourceHandle<T>::~ResourceHandle() noexcept {
        if (m_key.index != ~0U)
        ResourceManager::getInstance().release<T>(m_key, {});
    }

    template <typename T>
    const ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t) noexcept {
        if (ResourceManager::getInstance().contains<T>(m_key, {}))
            ResourceManager::getInstance().release<T>(m_key, {});

        m_key = null_key;

        return *this;
    }

    template <typename T>
    ResourceHandle<T>& ResourceHandle<T>::operator=(const ResourceHandle& rhs) noexcept {
        assert(ResourceManager::getInstance().contains<T>(rhs.m_key, {}));

        m_key = rhs.m_key;
        
        ResourceManager::getInstance().acquire<T>(m_key, {});

        return *this;
    }

    template <typename T>
    ResourceHandle<T>& ResourceHandle<T>::operator=(ResourceHandle&& rhs) noexcept {        
        if (this != &rhs) 
            std::swap(m_key, rhs.m_key);
        
        rhs = nullptr;

        return *this;
    }

    template <typename T>
    const SlotKey& ResourceHandle<T>::key() const noexcept {
        return m_key;
    }

    template <typename T>
    const ResourceState& ResourceHandle<T>::state() const noexcept {
        return ResourceManager::getInstance().getState<T>(m_key, {});
    }

    template <typename T>
    const T& ResourceHandle<T>::data() const {
        return ResourceManager::getInstance().get<T>(m_key, {});
    }

    template <typename T>
    template <typename... Args>
    void ResourceHandle<T>::recreate(Args&&... args) {
        ResourceManager::getInstance().recreate(*this, std::forward<Args>(args)..., {});
    }
}

#endif