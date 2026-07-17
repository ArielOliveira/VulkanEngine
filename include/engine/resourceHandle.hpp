#ifndef ENGINE_RESOURCE_HANDLE_HPP
#define ENGINE_RESOURCE_HANDLE_HPP

#include <engine/resourceTypes.hpp>

namespace Engine {
    template <typename T>
    class ResourceHandle {
        public:
            ResourceHandle() = delete;
            ResourceHandle(SlotKey key) noexcept;
            ResourceHandle(std::nullptr_t) noexcept;
            ResourceHandle(const ResourceHandle&) noexcept;
            ResourceHandle(ResourceHandle&& rhs) noexcept = default;

            ~ResourceHandle() noexcept;

            const ResourceHandle& operator=(std::nullptr_t) noexcept;
            const ResourceHandle& operator=(const ResourceHandle& rhs) = delete;
            ResourceHandle& operator=(ResourceHandle&& rhs) noexcept = default;

        private:
            SlotKey key;
    };
}

#endif