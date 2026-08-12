#include <engine/resourceEngine.hpp>

namespace Engine {
    ResourceManager& ResourceManager::getInstance() {
        static ResourceManager instance;

        return instance;
    }

    ResourceManager::ResourceManager() { 
        // Ensures order of initialization 
        // GPU memory allocator is guaranteed to be initialized by the time 
        // it is needed
        GPUResourceManager::getInstance(); 
    }

    ResourceManager::~ResourceManager() {}

    const ResourceHandle<Image> ResourceManager::createImage(const std::string& name, const TextureAsset& textureAsset, const VkImageCreateInfo& imageCreateInfo, const VkImageAspectFlags imageAspect) {
        assert(findKeyByName(std::type_index(typeid(Image)), name) == nullptr);

        VkImage image;
        VmaAllocation allocation;
        auto initialState = GPUResourceManager::getInstance().uploadData(
                image, allocation, 
                textureAsset.data, textureAsset.dataSize,
                textureAsset.layout,
                imageCreateInfo, 
                imageAspect);
        
        VkImageViewType viewType = textureAsset.layout.mips[0].depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : 
                           std::min(textureAsset.layout.mips[0].height, textureAsset.layout.mips[0].width) > 1 ? 
                           VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
            
        VkImageViewCreateInfo viewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image,
            .viewType         = viewType,
            .format           = imageCreateInfo.format,
            .subresourceRange = {.aspectMask = imageAspect, .baseMipLevel = 0, .levelCount = static_cast<uint32_t>(textureAsset.layout.mips.size()), .baseArrayLayer = 0, .layerCount = textureAsset.layout.layers}
        };
        
        VkImageView view;
        vkCreateImageView(*Core::getInstance().getDevice(), &viewCreateInfo, nullptr, &view);

        SlotKey key = pools.get<Image>().emplace(std::move( Image{
            .name        = name,
            .allocation  = allocation,
            .image       = image,
            .view        = view,
            .aspectFlags = imageAspect,
            .width       = imageCreateInfo.extent.width,
            .height      = imageCreateInfo.extent.height,
            .channels    = textureAsset.layout.channels,
            .mipCount    = static_cast<uint32_t>(textureAsset.layout.mips.size()),
            .layers      = textureAsset.layout.layers
        }));

        GPUResourceManager::getInstance().registerResourceState<ImageState>(key, initialState);
        
        cache[std::type_index(typeid(Image))][name] = key;

        return std::move(ResourceHandle<Image>(key));
    }

    const ResourceHandle<Image> ResourceManager::createImage(const std::string& name, const VkImageCreateInfo& imageCreateInfo, const VkImageAspectFlags imageAspect, const VkImageViewType viewType, const uint32_t mipCount, const uint32_t layers, const uint32_t channels) {
        assert(findKeyByName(std::type_index(typeid(Image)), name) == nullptr);

        VkImage image;
        VmaAllocation allocation;
        auto initialState = GPUResourceManager::getInstance().uploadData(image, allocation, imageCreateInfo);

         VkImageViewCreateInfo viewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image,
            .viewType         = viewType,
            .format           = imageCreateInfo.format,
            .subresourceRange = {.aspectMask = imageAspect, .baseMipLevel = 0, .levelCount = mipCount, .baseArrayLayer = 0, .layerCount = layers}
        };

        VkImageView view;
        vkCreateImageView(*Core::getInstance().getDevice(), &viewCreateInfo, nullptr, &view);

        SlotKey key = pools.get<Image>().emplace(std::move( Image{
            .name        = name,
            .allocation  = allocation,
            .image       = image,
            .view        = view,
            .aspectFlags = imageAspect,
            .width       = imageCreateInfo.extent.width,
            .height      = imageCreateInfo.extent.height,
            .channels    = channels,
            .mipCount    = mipCount,
            .layers      = layers
        }));

        GPUResourceManager::getInstance().registerResourceState<ImageState>(key, initialState);

        cache[std::type_index(typeid(Image))][name] = key;

        return std::move(ResourceHandle<Image>(key));
    }

    const ResourceHandle<Image> ResourceManager::createDepthAttachment(const uint32_t width, const uint32_t height) {
        auto depthFormat = static_cast<VkFormat>(Core::getInstance().findDepthFormat());
        auto imageType   = VkImageType::VK_IMAGE_TYPE_2D;
        auto viewType    = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        auto extent      = VkExtent3D { width, height, 1 };

        return createImage(
            std::string("DepthBuffer"), 
            { .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, 
              .imageType   = imageType, .format = depthFormat, 
              .extent      = extent, 
              .mipLevels   = 1, .arrayLayers = 1,
              .samples     = VK_SAMPLE_COUNT_1_BIT, 
              .tiling      = VK_IMAGE_TILING_OPTIMAL,
              .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
              .sharingMode = VK_SHARING_MODE_EXCLUSIVE },
            VK_IMAGE_ASPECT_DEPTH_BIT,  
            VK_IMAGE_VIEW_TYPE_2D,   
            1, 1, 1
        );
    }

    const ResourceHandle<Image> ResourceManager::createColorResolveAttachment(const uint32_t width, const uint32_t height, VkFormat format) {
        auto imageType   = VkImageType::VK_IMAGE_TYPE_2D;
        auto viewType    = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        auto extent      = VkExtent3D { width, height, 1 };

        return createImage(
            std::string("ResolveAttachment"), 
            { .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, 
              .imageType   = imageType, .format = format, 
              .extent      = extent, 
              .mipLevels   = 1, .arrayLayers = 1,
              .samples     = (VkSampleCountFlagBits)Core::getInstance().getMaxUsableSampleCount(), 
              .tiling      = VK_IMAGE_TILING_OPTIMAL,
              .usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
              .sharingMode = VK_SHARING_MODE_EXCLUSIVE },
            VK_IMAGE_ASPECT_DEPTH_BIT,  
            VK_IMAGE_VIEW_TYPE_2D,   
            1, 1, 1
        );
    }

    SlotKey* const ResourceManager::findKeyByName(const std::type_index& typeId, const std::string& name) { 
        auto it = cache.find(typeId);
        
        if (it == cache.end()) return nullptr;

        try {
            return &it->second.at(name);
        } catch(std::exception e) {
            return  nullptr;
        }
    }
}