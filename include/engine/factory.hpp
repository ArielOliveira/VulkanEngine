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

        const ImageMetaData createDepthAttachment(const uint32_t width, const uint32_t height, VkFormat depthFormat);
        const ImageMetaData createColorResolveAttachment(const uint32_t width, const uint32_t height, VkFormat format);
    };
}

#endif