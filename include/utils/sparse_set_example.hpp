// paged_component_pool.hpp
//
// A minimal, EnTT-style paged sparse set used to store one component type.
//
// Layout:
//   sparse[page][offset]  -> dense index (uint32_t), or TOMBSTONE if unused
//   dense[i]               -> entity that owns packed[i]
//   packed[i]               -> the component data for dense[i]
//
// `dense` and `packed` are always index-aligned and always tightly packed
// (no holes), which is what makes iterating this pool a plain linear scan.
// `sparse` is what's paged: it's an array of *pointers to pages*, and pages
// are only allocated once an entity in that page's index range actually
// gets inserted.
//
// Why page it at all instead of one flat sparse array sized to the max
// entity index? Two reasons that both come up in practice:
//   1. Entity ids aren't necessarily dense/contiguous (e.g. partitioned
//      ids for networked entities can be numerically far apart), so a flat
//      array would have to be sized to the *largest* id even if you only
//      have a handful of live entities.
//   2. Sparse-set backed storage (e.g. Flecs' "sparse components") wants
//      component addresses to be stable across inserts elsewhere in the
//      registry -- paging means growth only ever allocates a new page,
//      it never reallocates/moves existing pages the way a growing
//      std::vector would.
//
// This file assumes `Entity` is a plain unsigned integer index (no
// embedded generation/version bits). If you're using generational ids
// packed into a single integer (entity = index | version << N), mask off
// the version before calling into this pool -- paging only cares about
// the index portion. Your slotmap's generation check happens one layer up,
// same as it does today.

#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

template <typename Entity, typename Component, std::size_t PageSize = 4096>
class paged_component_pool {
    static_assert(std::is_unsigned_v<Entity>, "Entity must be an unsigned index type");
    static_assert((PageSize & (PageSize - 1)) == 0, "PageSize must be a power of two");

    using index_t = std::uint32_t;
    static constexpr index_t tombstone = static_cast<index_t>(-1);

    // log2(PageSize), computed at compile time so page/offset splitting
    // is a shift + mask instead of a division/modulo.
    static constexpr std::size_t page_bits = [] {
        std::size_t bits = 0;
        std::size_t size = PageSize;
        while (size > 1) { size >>= 1; ++bits; }
        return bits;
    }();
    static constexpr Entity page_mask = static_cast<Entity>(PageSize - 1);

    using page_t = std::array<index_t, PageSize>;

    std::vector<std::unique_ptr<page_t>> sparse_; // pages, allocated lazily
    std::vector<Entity> dense_;                    // packed entity ids
    std::vector<Component> packed_;                // packed component data (aligned with dense_)

    static constexpr std::size_t page_of(Entity e)   { return static_cast<std::size_t>(e) >> page_bits; }
    static constexpr std::size_t offset_of(Entity e) { return static_cast<std::size_t>(e & page_mask); }

    // Returns the page for `e`, allocating (and tombstone-filling) it if
    // this is the first entity we've seen in that page's range.
    page_t& assure_page(Entity e) {
        const std::size_t p = page_of(e);
        if (p >= sparse_.size()) {
            sparse_.resize(p + 1); // grows the *pointer* array only, pages themselves stay put
        }
        if (!sparse_[p]) {
            sparse_[p] = std::make_unique<page_t>();
            sparse_[p]->fill(tombstone);
        }
        return *sparse_[p];
    }

public:
    bool contains(Entity e) const {
        const std::size_t p = page_of(e);
        if (p >= sparse_.size() || !sparse_[p]) return false;
        return (*sparse_[p])[offset_of(e)] != tombstone;
    }

    // Assign / overwrite the component for `e`. Returns a reference to
    // the stored component so callers can do:
    //   auto& vel = pool.emplace(e, Velocity{1.f, 0.f});
    template <typename... Args>
    Component& emplace(Entity e, Args&&... args) {
        assert(!contains(e) && "entity already has this component");
        page_t& page = assure_page(e);
        page[offset_of(e)] = static_cast<index_t>(dense_.size());
        dense_.push_back(e);
        packed_.emplace_back(std::forward<Args>(args)...);
        return packed_.back();
    }

    // Swap-and-pop removal. Keeps dense_/packed_ tightly packed by moving
    // the *last* element into the freed slot -- same trick your slotmap
    // already uses, just applied to two parallel arrays instead of one.
    void erase(Entity e) {
        assert(contains(e) && "erasing an entity that has no component");
        const std::size_t p = page_of(e);
        const std::size_t o = offset_of(e);
        const index_t pos = (*sparse_[p])[o];
        const index_t last = static_cast<index_t>(dense_.size() - 1);

        if (pos != last) {
            Entity moved = dense_[last];
            dense_[pos] = moved;
            packed_[pos] = std::move(packed_[last]);
            (*sparse_[page_of(moved)])[offset_of(moved)] = pos;
        }

        dense_.pop_back();
        packed_.pop_back();
        (*sparse_[p])[o] = tombstone;
    }

    Component& get(Entity e) {
        assert(contains(e));
        return packed_[(*sparse_[page_of(e)])[offset_of(e)]];
    }

    const Component& get(Entity e) const {
        assert(contains(e));
        return packed_[(*sparse_[page_of(e)])[offset_of(e)]];
    }

    std::size_t size() const noexcept { return dense_.size(); }
    bool empty() const noexcept { return dense_.empty(); }

    // Linear iteration over packed component data -- this is the payoff
    // of keeping dense_/packed_ tightly packed: no tombstones to skip,
    // no branching, just a straight scan.
    auto begin()       { return packed_.begin(); }
    auto end()         { return packed_.end(); }
    auto begin() const { return packed_.begin(); }
    auto end() const   { return packed_.end(); }

    // Access to the owning entity for a given packed index, useful when
    // iterating packed_ directly and needing to know "whose component is
    // this" (e.g. dense_[i] owns packed_[i]).
    const std::vector<Entity>& entities() const noexcept { return dense_; }
};