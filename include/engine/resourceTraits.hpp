#ifndef ENGINE_RESOURCE_TRAITS_HPP
#define ENGINE_RESOURCE_TRAITS_HPP

namespace Engine::Resources::Traits {
    using namespace Engine::Resources::GPU;

    template <typename... Ts>
    struct TypeList {};

    using GpuResourceTypes = TypeList<GPU::Buffer, GPU::Image>;

    template <typename T, typename List>
    struct TypeListContains;

    template <typename T, typename... Ts>
    struct TypeListContains<T, TypeList<Ts...>>
        : std::disjunction<std::is_same<T, Ts>...> {};

    template <typename T>
    inline constexpr bool is_gpu_resource_v = TypeListContains<T, GpuResourceTypes>::value;

    template <typename T>
    struct ResourceTraits {
        static auto& manager() {
            if constexpr (is_gpu_resource_v<T>) 
                return GPUResourceManager::getInstance();
            else
                return ResourceManager::getInstance();
        }
    };
}

#endif
