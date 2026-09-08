#ifndef ENGINE_FACTORY
#define ENGINE_FACTORY

#include <tuple>
#include <string>
#include <utility>

#include <vk_mem_alloc.h>

using std::string;
using std::tuple;

namespace Engine::Factory {
    namespace ImageFTY {
        using ImageMetaData = tuple<VkImageCreateInfo, VkImageAspectFlags>;

        const ImageMetaData createDepthAttachmentInfo(const uint32_t width, const uint32_t height, VkFormat depthFormat);
        const ImageMetaData createColorResolveAttachmentInfo(const uint32_t width, const uint32_t height, VkFormat format);

        const VkImageViewCreateInfo createImageViewInfo(const VkImage& image, const VkImageAspectFlags aspect, const VkFormat format, 
                                                        const uint32_t width, const uint32_t height, const uint32_t depth,
                                                        const uint32_t mipCount, const uint32_t layerCount);
    }

    namespace ArrayFTY {
        template <typename T, size_t... Is>
        static constexpr std::array<T, sizeof...(Is)> make_null_array_impl(std::index_sequence<Is...>) {
            return { T(((void)(Is), nullptr))... };
        }

        template <typename T, size_t N>
        inline constexpr std::array<T, N> make_null_array() {
            return make_null_array_impl<T>(std::make_index_sequence<N>{});
        }
    }
}

#endif