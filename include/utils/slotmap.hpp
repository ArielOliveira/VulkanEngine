#ifndef UTILS_SLOT_MAP_HPP
#define UTILS_SLOT_MAP_HPP

#include <iostream>
#include <vector>

#include <utils/types.hpp>

using std::vector;

using namespace Utils::Types;

namespace Utils {
    // Slot map implementation:
    // This implementation uses a free-list. Over time
    // insertion and allocation will happen at random indexes
    // hurting cache locality. 
    // Possible alternative: https://github.com/sporacid/slot-map/blob/main/include/spore/slot_map.hpp
    //                     : https://jakubtomsu.github.io/posts/bit_pools/
    template <typename T, typename Slot = SlotBase, size_t PageSize = 4, bool AllowReallocation = true>
    class SlotMap {
        public:
            SlotMap();
            ~SlotMap();    

            SlotMap(const SlotMap&) = delete;
            SlotMap& operator=(const SlotMap&) = delete;

            SlotMap(SlotMap&&) noexcept = default;
            SlotMap& operator=(SlotMap&&) noexcept = default;
            
            template <typename... Args>
            const SlotKey emplace(Args&&... args);

            void erase(const SlotKey& key);
            
            const T*     getData()  const;
            const bool   contains(const SlotKey& key) const;
            const size_t size()     const;
            const size_t capacity() const;
            
            Slot& getSlot(const SlotKey& key);
            const Slot& getSlot(const SlotKey& key) const;
            
            const T& operator[](const SlotKey& key) const;
            T& operator[](const SlotKey& key);
        private:
            static_assert((PageSize & (PageSize-1)) == 0, "PageSize must be a power of two");

            uint32_t freeHead;
            uint32_t freeTail;
            
            vector<T>        data;
            vector<Slot>     slots;
            vector<uint32_t> eraseTable;

            SlotHandler<Slot> slotHandler;

            const uint32_t popFreeList() noexcept;
            void pushFreeList(const uint32_t keyIndex)  noexcept;
            void reallocate();
        };
}

#include "utils/slotmap.tpp"

#endif