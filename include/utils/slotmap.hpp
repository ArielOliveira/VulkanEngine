#ifndef SLOT_MAP_HPP
#define SLOT_MAP_HPP

#include <iostream>
#include <vector>

using std::vector;

namespace Utils {
    struct SlotKey {
        uint32_t index;
        uint32_t generations;
    };

    // Slot map implementation:
    // This implementation uses a free-list. Over time
    // insertion and allocation will happen at random indexes
    // hurting cache locality. 
    // Possible alternative: https://github.com/sporacid/slot-map/blob/main/include/spore/slot_map.hpp
    //                     : https://jakubtomsu.github.io/posts/bit_pools/
    template <typename T>
    class SlotMap {
        public:
            SlotMap(uint32_t allocationChunkSize = 8, bool allowReallocation = true);
            ~SlotMap();    
            
            const SlotKey insert(const T &value);
            void erase(const SlotKey& key);
            
            const T*     getRawData()  const;
            
            const size_t getSize()     const;
            const size_t getCapacity() const;
        private:
            uint32_t allocationChunkSize;
            uint32_t freeHead;
            uint32_t freeTail;
            
            vector<T>        data;
            vector<SlotKey>  slots;
            vector<uint32_t> eraseTable;

            bool allowReallocation;
            
            static constexpr uint32_t MIN_ALLOCATION_CHUNK = 2;

            const uint32_t pushFreeList() noexcept;
            void popFreeList(const uint32_t keyIndex)  noexcept;
            void reallocate();
        };
}

#include "utils/slotmap.tpp"

#endif