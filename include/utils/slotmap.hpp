#ifndef SLOT_MAP_HPP
#define SLOT_MAP_HPP

#include <iostream>
#include <vector>

using std::vector;

namespace Utils {
    struct Key {
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
            SlotMap(uint32_t allocationChunkSize = 8);
            ~SlotMap();    
            
            const Key insert(const T &value) noexcept;
            void erase(const Key& key)      ;

            const T*     getRawData()  const;
            
            const size_t getSize()     const;
            const size_t getCapacity() const;
        private:
            uint32_t allocationChunkSize;
            uint32_t freeHead;
            uint32_t freeTail;
            
            vector<T>        data;
            vector<Key>      slots;
            vector<uint32_t> eraseTable;
            
            static constexpr uint32_t MIN_ALLOCATION_CHUNK = 2;

            const uint32_t pushFreeList() noexcept;
            void popFreeList(const uint32_t keyIndex)  noexcept;
            void reallocate() noexcept;
        };
}

#include "utils/slotmap.tpp"

#endif