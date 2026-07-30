#ifndef UTILS_SPARSE_SET_HPP
#define UTILS_SPARSE_SET_HPP

#include <iostream>
#include <array>
#include <memory>
#include <vector>

#include <utils/types.hpp>

using std::array;
using std::vector;
using std::unique_ptr;
using std::make_unique;

using namespace Utils::Types;

namespace Utils {
    template <typename T, typename Entity = SlotBase, size_t PageSize = 256>
    class SparseSet {
        public:
            SparseSet();
            ~SparseSet();   
            
            const bool contains(const Entity& e) const;
            
            template <typename... Args>
            T& emplace(const Entity& e, Args&&... args);
            void erase(const Entity& e);
            
            T& operator[](const Entity& e);
            const T& operator[](const Entity& e) const;

            const Entity& operator[](uint32_t index) const;

            const size_t size()     const { return dense.size();     }
            const bool  empty()     const { return dense.empty();    }

            auto begin() { return packed.begin(); }
            auto end()   { return packed.end();   }
            auto begin() const { return packed.begin(); }
            auto end()   const { return packed.end();   }
        private:      
            static_assert((PageSize & (PageSize-1)) == 0, "PageSize must be a power of two");
            
            using Page = array<uint32_t, PageSize>;

            Page& getPage(const Entity& e);

            static constexpr uint32_t TOMBSTONE = ~0U;
            static constexpr size_t   PAGE_MASK = PageSize-1;

            static constexpr size_t PAGE_BITS = [] {
                size_t bits = 0;
                size_t size = PageSize;

                while(size > 1) { size >>= 1; bits++; }

                return bits;
            }();

            static constexpr size_t page(uint32_t index)   { return static_cast<size_t>(index) >> PAGE_BITS; }
            static constexpr size_t offset(uint32_t index) { return static_cast<size_t>(index) &  PAGE_MASK; }
    
            vector<unique_ptr<Page>> sparse;
            vector<Entity>           dense;
            vector<T>                packed;
        };
}

#include "utils/sparseset.tpp"

#endif