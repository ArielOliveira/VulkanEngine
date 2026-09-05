#ifndef SLOT_MAP_TPP
#define SLOT_MAP_TPP

#include <utils/slotmap.hpp>
#include <cassert>

namespace Utils {
    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    SlotMap<T, Slot, PageSize, AllowReallocation>::SlotMap() {
        data.reserve(PageSize);
        eraseTable.reserve(PageSize);
        slots.reserve(PageSize);

        for (uint32_t i = 0; i < PageSize-1U; i++) 
            slots.emplace_back( Slot { i+1U, 0 }); // next available slot

        slots.emplace_back( Slot { ~0U, 0 });

        freeHead              = 0;
        freeTail              = PageSize-1U;
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    SlotMap<T, Slot, PageSize, AllowReallocation>::~SlotMap() {}

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    template <typename... Args>
    const SlotKey SlotMap<T, Slot, PageSize, AllowReallocation>::emplace(Args&&... args) {
        if (freeHead == ~0U) {
            if constexpr (AllowReallocation)
                reallocate();
            else
                throw std::runtime_error("Attempting to allocate on filled SlotMap with AllowReallocation = false");
        }
        
        uint32_t slotKey = popFreeList();
        slotHandler.reinsert(slots[slotKey], static_cast<uint32_t>(data.size()));
        
        data.emplace_back(std::forward<Args>(args)...);
        eraseTable.push_back(slotKey);

        return { slotKey, slots[slotKey].generations };
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    void SlotMap<T, Slot, PageSize, AllowReallocation>::erase(const SlotKey &key) {
        if (key.index < 0 || key.index >= slots.size())
            throw std::runtime_error("Index out of bounds!");

        if (key.generations != slots[key.index].generations)
            throw std::runtime_error("Invalid key! Possible use-after-free.");

        uint32_t dataIndex = slotHandler.recycle(slots[key.index]).index;
        
        std::swap(data[dataIndex], data.back());
        std::swap(eraseTable[dataIndex], eraseTable.back());

        data.pop_back();
        eraseTable.pop_back();

        if (dataIndex < data.size())
            slots[eraseTable[dataIndex]].index = dataIndex;
        
        pushFreeList(key.index);
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    void SlotMap<T, Slot, PageSize, AllowReallocation>::reallocate() {
        static_assert(AllowReallocation, "Trying to reallocate on locked SlotMap");

        uint32_t oldSize = static_cast<uint32_t>(slots.size());
        uint32_t newSize = oldSize + PageSize;

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

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    const uint32_t SlotMap<T, Slot, PageSize, AllowReallocation>::popFreeList() noexcept {
        uint32_t slotKey = freeHead;    
        freeHead = slots[freeHead].index;

        return slotKey;
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    void SlotMap<T, Slot, PageSize, AllowReallocation>::pushFreeList(const uint32_t keyIndex) noexcept {
        uint32_t previousTail = freeTail;
        freeTail = keyIndex;

        if (freeHead != ~0U) 
            slots[previousTail].index = freeTail;
        else 
            freeHead = freeTail;   

        slots[freeTail].index = ~0U;
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    Slot& SlotMap<T, Slot, PageSize, AllowReallocation>::getSlot(const SlotKey& key) { 
        assert(key.index >= 0 && !(key.index >= slots.size()));
        assert(key.generations == slots[key.index].generations);

        return slots[key.index]; 
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    const Slot& SlotMap<T, Slot, PageSize, AllowReallocation>::getSlot(const SlotKey& key) const {
        assert(key.index >= 0 && !(key.index >= slots.size()));
        assert(key.generations == slots[key.index].generations);

        return slots[key.index]; 
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    const bool SlotMap<T, Slot, PageSize, AllowReallocation>::contains(const SlotKey& key) const {
        return key.index >= 0 && 
               key.index < slots.size() && 
               slots[key.index].generations == key.generations;
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    const T* SlotMap<T, Slot, PageSize, AllowReallocation>::getData() const { return data.data(); }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    const size_t SlotMap<T, Slot, PageSize, AllowReallocation>::size() const { return data.size(); }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    const size_t SlotMap<T, Slot, PageSize, AllowReallocation>::capacity() const { return data.capacity(); }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    const T& SlotMap<T, Slot, PageSize, AllowReallocation>::operator[](const SlotKey& key) const {
        assert(key.index >= 0 && !(key.index >= slots.size()));
        assert(key.generations == slots[key.index].generations);

        return data[slots[key.index].index];
    }

    template <typename T, typename Slot, size_t PageSize, bool AllowReallocation>
    T& SlotMap<T, Slot, PageSize, AllowReallocation>::operator[](const SlotKey& key) {
        assert(key.index >= 0 && !(key.index >= slots.size()));
        assert(key.generations == slots[key.index].generations);

        return data[slots[key.index].index];
    }
}

#endif