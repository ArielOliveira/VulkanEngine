#ifndef UTILS_TYPES_HPP
#define UTILS_TYPES_HPP

#include <cassert>
#include <cstdint>
#include <atomic>

namespace Utils::Types {
    enum class ResourceState {
        None      = 0,
        Pending   = 1,
        Loading   = 2,
        Ready     = 3,
        Releasing = 4,
    };

    struct SlotBase {
        uint32_t index;
        uint32_t generations;

        const bool operator==(const SlotBase& rhs) const { return index == rhs.index && generations == rhs.generations; }
    };

    struct ResourceSlot : SlotBase {
        std::atomic<ResourceState> state = ResourceState::None;
        std::atomic<uint32_t> refCount = 0;

        ResourceSlot(uint32_t&& index, uint32_t&& generations) noexcept : SlotBase(std::move(index), std::move(generations)) {}

        ResourceSlot(SlotBase&& rhs) noexcept : SlotBase(std::move(rhs)) {}

        ResourceSlot(ResourceSlot&& rhs) noexcept 
        : SlotBase(std::move(rhs))
        , state(rhs.state.load()) 
        , refCount(rhs.refCount.load()) {}
        
        ResourceSlot& operator=(ResourceSlot&& rhs) noexcept {
            if (this != &rhs) {
                std::swap(index, rhs.index);
                std::swap(generations, rhs.generations);

                state.store(rhs.state.load());
                refCount.store(rhs.refCount.load());
            }

            return *this;
        }

        ResourceSlot(const ResourceSlot&) = delete;
        ResourceSlot& operator=(const ResourceSlot&) = delete;

    };

    template <typename SlotType = SlotBase>
    struct SlotHandler {
        const SlotType& reinsert(SlotType& slot, const uint32_t newKey) { 
            slot.index = newKey; 
            slot.generations++; 

            return slot;
        }

        const SlotType& recycle(SlotType& slot) {
            slot.generations++;

            return slot;
        }
    };


    template<>
    inline const ResourceSlot& SlotHandler<ResourceSlot>::reinsert(ResourceSlot& slot, const uint32_t newKey) {
        slot.index = newKey;
        slot.generations++;
            
        assert(slot.refCount == 0 && slot.state == ResourceState::None);

        slot.state = ResourceState::Pending; 

        return slot;
    }

    template<>
    inline const ResourceSlot& SlotHandler<ResourceSlot>::recycle(ResourceSlot& slot) {
        slot.generations++;

        assert(slot.refCount == 0 && slot.state == ResourceState::Releasing);

        slot.state  = ResourceState::None;

        return slot;
    }

    typedef SlotBase SlotKey;
}

#endif