#ifndef SLOT_MAP_TPP
#define SLOT_MAP_TPP

#include <utils/slotmap.hpp>

namespace Utils {
    template <typename T>
    SlotMap<T>::SlotMap(uint32_t allocationChunkSize) {
        allocationChunkSize = std::max(allocationChunkSize, MIN_ALLOCATION_CHUNK);

        this->allocationChunkSize = allocationChunkSize;

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
    const Key SlotMap<T>::insert(const T &value) noexcept {
        if (freeHead == ~0U) 
            reallocate();
        
        uint32_t slotKey = pushFreeList();    

        slots[slotKey].index = slotKey;
        slots[slotKey].generations++;
        
        data.push_back(value);
        eraseTable.push_back(slotKey);

        return { slotKey, slots[slotKey].generations };
    }

    template <typename T>
    void SlotMap<T>::erase(const Key &key) {
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
    void SlotMap<T>::reallocate() noexcept {
        std::cout << "Reallocation not implemented!" << '\n';

        std::terminate();
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