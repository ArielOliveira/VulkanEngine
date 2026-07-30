#ifndef UTILS_TYPES_HPP
#define UTILS_TYPES_HPP

#include <cstdint>

namespace Utils::Types {
    struct SlotBase {
        uint32_t index;
        uint32_t generations;

        const bool operator==(const SlotBase& rhs) const { return index == rhs.index && generations == rhs.generations; }
    };

    struct ResourceSlot : SlotBase {
        uint32_t refCount = 0;
    };

    typedef SlotBase SlotKey;
}

#endif