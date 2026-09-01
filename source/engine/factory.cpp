#include <engine/factory.hpp>
#include <graphics/core.hpp>

using Graphics::Core;

namespace Engine::Factory {
    namespace ImageFTY {
        const ImageMetaData createDepthAttachment(const uint32_t width, const uint32_t height, VkFormat depthFormat) {
            return {
                { .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, 
                  .imageType   = VkImageType::VK_IMAGE_TYPE_2D, 
                  .format      = depthFormat, 
                  .extent      = { width, height, 1 }, 
                  .mipLevels   = 1, .arrayLayers = 1,
                  .samples     = (VkSampleCountFlagBits)Core::getInstance().getMaxUsableSampleCount(), 
                  .tiling      = VK_IMAGE_TILING_OPTIMAL,
                  .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE },
                VK_IMAGE_ASPECT_DEPTH_BIT
            };
        }
        
        const ImageMetaData createColorResolveAttachment(const uint32_t width, const uint32_t height, VkFormat format) {
            return {
                { .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, 
                .imageType   = VkImageType::VK_IMAGE_TYPE_2D, 
                .format      = format, 
                .extent      = { width, height, 1 }, 
                .mipLevels   = 1, .arrayLayers = 1,
                .samples     = (VkSampleCountFlagBits)Core::getInstance().getMaxUsableSampleCount(), 
                .tiling      = VK_IMAGE_TILING_OPTIMAL,
                .usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE },
                VK_IMAGE_ASPECT_COLOR_BIT
            };
        }
    }
}