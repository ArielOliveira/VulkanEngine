#ifndef ENGINE_RESOURCE_HANDLE_TPP
#define ENGINE_RESOURCE_HANDLE_TPP

#include <engine/resourceHandle.hpp>

namespace Engine {
    using namespace Resources::Traits;

    template <typename T>
    ResourceHandle<T>::ResourceHandle(std::nullptr_t) noexcept : m_key(null_key) {}

    template <typename T>
    ResourceHandle<T>::ResourceHandle(SlotKey key) noexcept : m_key(key) {
        assert(m_key.index != ~0 && ResourceTraits<T>::manager().template contains<T>(m_key, {}));

        ResourceTraits<T>::manager().template acquire<T>(key, {});
    }

    /*template <typename T>
    ResourceHandle<T>::ResourceHandle(const ResourceHandle& rhs) noexcept : m_key(rhs.m_key) {
        assert(m_key.index != ~0 && ResourceTraits<T>::manager().template contains<T>(m_key, {}));

        ResourceTraits<T>::manager().template acquire<T>(m_key, {});
    }*/

    template <typename T>
    ResourceHandle<T>::ResourceHandle(ResourceHandle&& rhs) noexcept { std::swap(m_key, rhs.m_key); }

    template <typename T>
    ResourceHandle<T>::~ResourceHandle() noexcept {
        if (m_key.index != ~0U)
            ResourceTraits<T>::manager().template release<T>(m_key, {});
    }

    template <typename T>
    const ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t) noexcept {
        if (ResourceTraits<T>::manager().template contains<T>(m_key, {}))
            ResourceTraits<T>::manager().template release<T>(m_key, {});

        m_key = null_key;

        return *this;
    }

    /*template <typename T>
    ResourceHandle<T>& ResourceHandle<T>::operator=(const ResourceHandle& rhs) noexcept {
        assert(ResourceTraits<T>::manager().template contains<T>(rhs.m_key, {}));

        *this = nullptr;

        m_key = rhs.m_key;
        
        ResourceTraits<T>::manager().template acquire<T>(m_key, {});

        return *this;
    }*/

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
        return ResourceTraits<T>::manager().template getState<T>(m_key, {});
    }

    template <typename T>
    const T& ResourceHandle<T>::data() const {
        return ResourceTraits<T>::manager().template get<T>(m_key, {});
    }

    /*template <typename T>
    template <typename... Args>
    void ResourceHandle<T>::recreate(Args&&... args) {
        ResourceTraits<T>::manager().recreate(*this, std::forward<Args>(args)..., {});
    }*/
}

#endif