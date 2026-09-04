#ifndef ENGINE_FACTORY
#define ENGINE_FACTORY

#include <tuple>
#include <string>

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
    };
}

#endif