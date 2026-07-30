#ifndef UTILS_SPARSE_SET_TPP
#define UTILS_SPARSE_SET_TPP

#include <utils/sparseset.hpp>

namespace Utils {
    template <typename T, typename Entity, size_t PageSize>
    SparseSet<T, Entity, PageSize>::SparseSet() {}

    template <typename T, typename Entity, size_t PageSize>
    SparseSet<T, Entity, PageSize>::~SparseSet() {}

    template <typename T, typename Entity, size_t PageSize>
    const bool SparseSet<T, Entity, PageSize>::contains(const Entity& e) const {
        const size_t p = page(e.index);
        if (p >= sparse.size() || !sparse[p]) return false;
        const uint32_t pos = (*sparse[p])[offset(e.index)];

        return pos != TOMBSTONE && dense[pos] == e;
    }

    template <typename T, typename Entity, size_t PageSize>
    SparseSet<T, Entity, PageSize>::Page& SparseSet<T, Entity, PageSize>::getPage(const Entity& e) {
        const size_t p = page(e.index);

        if (p >= sparse.size()) 
            sparse.resize(p + 1);

        if (!sparse[p]) {
            sparse[p] = make_unique<SparseSet<T, Entity, PageSize>::Page>();
            sparse[p]->fill(TOMBSTONE);
        }

        return *sparse[p];
    }

    template <typename T, typename Entity, size_t PageSize>
    template <typename... Args>
    T& SparseSet<T, Entity, PageSize>::emplace(const Entity& e, Args&&... args) {
        assert(!contains(e) && "Entity already has this component!");

        auto& page = getPage(e);
        page[offset(e.index)] = static_cast<uint32_t>(dense.size());

        dense.push_back(e);
        packed.emplace_back(std::forward<Args>(args)...);
        
        return packed.back();
    }

    template <typename T, typename Entity, size_t PageSize>
    void SparseSet<T, Entity, PageSize>::erase(const Entity& e) {
        assert(contains(e) && "Erasing a component that doesn't exist");
        const size_t pageIdx    = page(e.index);
        const size_t pageOffset = offset(e.index);

        const uint32_t pos  = (*sparse[pageIdx])[pageOffset];
        const uint32_t last = static_cast<uint32_t>(dense.size()-1);

        if (pos != last) {
            uint32_t moved = dense[last].index;

            std::swap(dense[pos], dense.back());
            std::swap(packed[pos], packed.back());

            (*sparse[page(moved)])[offset(moved)] = pos;
        }

        dense.pop_back();
        packed.pop_back();
        (*sparse[pageIdx])[pageOffset] = TOMBSTONE;
    }

    template <typename T, typename Entity, size_t PageSize>
    T& SparseSet<T, Entity, PageSize>::operator[](const Entity& e) {
        assert(contains(e) && "Entity does not have this component");

        return packed[(*sparse[page(e.index)])[offset(e.index)]];
    }

    template <typename T, typename Entity, size_t PageSize>
    const T& SparseSet<T, Entity, PageSize>::operator[](const Entity& e) const {
        assert(contains(e) && "Entity does not have this component");

        return packed[(*sparse[page(e.index)])[offset(e.index)]];
    }

    template <typename T, typename Entity, size_t PageSize>
    const Entity& SparseSet<T, Entity, PageSize>::operator[](uint32_t index) const {
        assert(index >= 0 && index < static_cast<uint32_t>(dense.size()));

        return dense[index];
    }
}

#endif