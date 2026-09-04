#ifndef ENGINE_RESOURCE_HANDLE
#define ENGINE_RESOURCE_HANDLE

#include <utils/slotmap.hpp>

namespace Engine {
    template <typename T>
    class ResourceHandle {
        public:
            ResourceHandle() = delete;
            ResourceHandle(SlotKey key) noexcept;
            ResourceHandle(std::nullptr_t) noexcept;
            ResourceHandle(const ResourceHandle&) noexcept;
            ResourceHandle(ResourceHandle&& rhs) noexcept;

            ~ResourceHandle() noexcept;

            const ResourceHandle& operator=(std::nullptr_t) noexcept;
            ResourceHandle& operator=(const ResourceHandle& rhs) noexcept;
            ResourceHandle& operator=(ResourceHandle&& rhs) noexcept;
            const ResourceHandle& operator=(const ResourceHandle& rhs) = delete;

            const SlotKey& key() const noexcept;
            const ResourceState& state() const noexcept;

            const T& data() const;
            
            template <typename... Args>
            void recreate(Args&&... args);

        private:
            static constexpr SlotKey null_key = { ~0U, ~0U };

            SlotKey m_key = null_key;
    };
}

#endif