#ifndef SLOT_MAP_TPP
#define SLOT_MAP_TPP

#include <utils/slotmap.hpp>
#include <cassert>

namespace Utils {
    template <typename T, typename Slot>
    SlotMap<T, Slot>::SlotMap(uint32_t allocationChunkSize, bool allowReallocation) {
        allocationChunkSize = std::max(allocationChunkSize, MIN_ALLOCATION_CHUNK);

        this->allocationChunkSize = allocationChunkSize;
        this->allowReallocation   = allowReallocation;

        data.reserve(allocationChunkSize);
        eraseTable.reserve(allocationChunkSize);
        slots.reserve(allocationChunkSize);

        for (uint32_t i = 0; i < allocationChunkSize-1U; i++) 
            slots.emplace_back( Slot { i+1U, 0 }); // next available slot

        slots.emplace_back( Slot { ~0U, 0 });

        freeHead              = 0;
        freeTail              = allocationChunkSize-1U;
    }

    template <typename T, typename Slot>
    SlotMap<T, Slot>::~SlotMap() {}

    template <typename T, typename Slot>
    template <typename... Args>
    const SlotKey SlotMap<T, Slot>::emplace(Args&&... args) {
        if (freeHead == ~0U) {
            if (allowReallocation)
                reallocate();
            else
                return { ~0U, ~0U };
        }
        
        uint32_t slotKey = pushFreeList();    
        Slot& slot = slots[slotKey];

        slot = { slotKey, slot.generations+1 };
        
        data.emplace_back(std::forward<Args>(args)...);
        eraseTable.push_back(slotKey);

        return { slotKey, slot.generations };
    }

    template <typename T, typename Slot>
    void SlotMap<T, Slot>::erase(const SlotKey &key) {
        if (key.index < 0 || key.index >= slots.size())
            throw std::runtime_error("Index out of bounds!");

        if (key.generations != slots[key.index].generations)
            throw std::runtime_error("Invalid key! Possible use-after-free.");

        Slot& slot = slots[key.index];
        uint32_t dataIndex = slot.index;
        slot.generations++;

        std::swap(data[dataIndex], data.back());
        std::swap(eraseTable[dataIndex], eraseTable.back());

        data.pop_back();
        eraseTable.pop_back();

        if (dataIndex < data.size())
            slots[eraseTable[dataIndex]].index = dataIndex;
        
        popFreeList(key.index);
    }

    template <typename T, typename Slot>
    void SlotMap<T, Slot>::reallocate() {
        uint32_t oldSize = static_cast<uint32_t>(slots.size());
        uint32_t newSize = oldSize + allocationChunkSize;

        data.reserve(newSize);
        eraseTable.reserve(newSize);
        slots.reserve(newSize);

        for (uint32_t i = oldSize; i < newSize-1U; i++) 
            slots.emplace_back( Slot { i+1U, 0 } );

        slots.emplace_back ( Slot { ~0U, 0 } );

        if (freeHead == ~0U) 
            freeHead = oldSize;   
        else 
            slots[freeTail].index = oldSize; 
        
        freeTail = newSize-1U;
    }

    template <typename T, typename Slot>
    const uint32_t SlotMap<T, Slot>::pushFreeList() noexcept {
        uint32_t slotKey = freeHead;    
        freeHead = slots[freeHead].index;

        return slotKey;
    }

    template <typename T, typename Slot>
    void SlotMap<T, Slot>::popFreeList(const uint32_t keyIndex) noexcept {
        uint32_t previousTail = freeTail;
        freeTail = keyIndex;

        if (freeHead != ~0U) 
            slots[previousTail].index = freeTail;
        else 
            freeHead = freeTail;   

        slots[freeTail].index = ~0U;
    }

    template <typename T, typename Slot>
    Slot& SlotMap<T, Slot>::getSlot(const SlotKey& key) { 
        assert(key.index >= 0 && !(key.index >= slots.size()));
        assert(key.generations == slots[key.index].generations);

        return slots[key.index]; 
    }

    template <typename T, typename Slot>
    const Slot& SlotMap<T, Slot>::getSlot(const SlotKey& key) const {
        assert(key.index >= 0 && !(key.index >= slots.size()));
        assert(key.generations == slots[key.index].generations);

        return slots[key.index]; 
    }

    template <typename T, typename Slot>
    const T* SlotMap<T, Slot>::getData() const { return data.data(); }

    template <typename T, typename Slot>
    const size_t SlotMap<T, Slot>::size() const { return data.size(); }

    template <typename T, typename Slot>
    const size_t SlotMap<T, Slot>::capacity() const { return data.capacity(); }

    template <typename T, typename Slot>
    const T& SlotMap<T, Slot>::operator[](const SlotKey& key) const {
        assert(key.index >= 0 && !(key.index >= slots.size()));
        assert(key.generations == slots[key.index].generations);

        return data[slots[key.index].index];
    }
}

#endif