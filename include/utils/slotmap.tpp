#ifndef SLOT_MAP_TPP
#define SLOT_MAP_TPP

#include <utils/slotmap.hpp>

namespace Utils {
    template <typename T>
    SlotMap<T>::SlotMap(uint32_t allocationChunkSize, bool allowReallocation) {
        allocationChunkSize = std::max(allocationChunkSize, MIN_ALLOCATION_CHUNK);

        this->allocationChunkSize = allocationChunkSize;
        this->allowReallocation   = allowReallocation;

        data.reserve(allocationChunkSize);
        eraseTable.reserve(allocationChunkSize);
        
        slots.resize(allocationChunkSize);

        for (uint32_t i = 0; i < allocationChunkSize-1U; i++) 
            slots[i].index  = i+1U; // next available slot

        slots[allocationChunkSize-1U].index = ~0U;

        freeHead              = 0;
        freeTail              = allocationChunkSize-1U;
    }

    template <typename T>
    SlotMap<T>::~SlotMap() {}

    template <typename T>
    const SlotKey SlotMap<T>::insert(const T &value) {
        if (freeHead == ~0U) {
            if (allowReallocation)
                reallocate();
            else
                return { ~0U, ~0U };
        }
        
        uint32_t slotKey = pushFreeList();    

        slots[slotKey].index = slotKey;
        slots[slotKey].generations++;
        
        data.push_back(value);
        eraseTable.push_back(slotKey);

        return { slotKey, slots[slotKey].generations };
    }

    template <typename T>
    void SlotMap<T>::erase(const SlotKey &key) {
        if (key.index < 0 || key.index >= slots.size())
            throw std::runtime_error("Index out of bounds!");

        if (key.generations != slots[key.index].generations)
            throw std::runtime_error("Invalid key! Possible use-after-free.");

        uint32_t dataIndex = slots[key.index].index;
        slots[key.index].generations++;

        std::swap(data[key.index], data.back());
        std::swap(eraseTable[key.index], eraseTable.back());

        data.pop_back();
        eraseTable.pop_back();

        slots[eraseTable[key.index]].index = key.index;
        
        popFreeList(key.index);
    }

    template <typename T>
    void SlotMap<T>::reallocate() {
        uint32_t oldSize = static_cast<uint32_t>(slots.size());
        uint32_t newSize = oldSize + allocationChunkSize;

        data.reserve(newSize);
        eraseTable.reserve(newSize);
        slots.reserve(newSize);

        for (uint32_t i = oldSize; i < newSize-1U; i++) 
            slots.push_back( { i+1U, 0 } );

        slots.push_back ( { ~0U, 0 } );

        if (freeHead == ~0U) 
            freeHead = oldSize;   
        else 
            slots[freeTail].index = oldSize; 
        
        freeTail = newSize-1U;
    }

    template <typename T>
    const uint32_t SlotMap<T>::pushFreeList() noexcept {
        uint32_t slotKey = freeHead;    
        freeHead = slots[freeHead].index;

        return slotKey;
    }

    template <typename T>
    void SlotMap<T>::popFreeList(const uint32_t keyIndex) noexcept {
        uint32_t previousTail = freeTail;
        freeTail = keyIndex;

        if (freeHead != ~0U) 
            slots[previousTail].index = freeTail;
        else 
            freeHead = freeTail;   

        slots[freeTail].index = ~0U;
    }

    template <typename T>
    const T* SlotMap<T>::getRawData() const { return data.data(); }

    template <typename T>
    const size_t SlotMap<T>::getSize() const { return data.size(); }

    template <typename T>
    const size_t SlotMap<T>::getCapacity() const { return data.capacity(); }
}

#endif