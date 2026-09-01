#include <engine/resourceEngine.hpp>
#include <vulkan/utility/vk_format_utils.h>

using namespace Engine::Resources;

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
            .depth       = imageCreateInfo.extent.depth,
            .channels    = textureAsset.layout.channels,
            .mipCount    = static_cast<uint32_t>(textureAsset.layout.mips.size()),
            .layers      = textureAsset.layout.layers
        }));

        GPUResourceManager::getInstance().registerResourceState<ImageState>(key, initialState);
        
        cache[std::type_index(typeid(Image))][name] = key;

        return std::move(ResourceHandle<Image>(key));
    }

    const ResourceHandle<Image> ResourceManager::createImage(const std::string& name, const VkImageCreateInfo& imageCreateInfo, const VkImageAspectFlags imageAspect) {
        assert(findKeyByName(std::type_index(typeid(Image)), name) == nullptr);

        VkImage image;
        VmaAllocation allocation;
        auto initialState = GPUResourceManager::getInstance().uploadData(image, allocation, imageCreateInfo);

        VkImageViewType viewType = imageCreateInfo.extent.depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : 
                           std::min(imageCreateInfo.extent.height, imageCreateInfo.extent.width) > 1 ? 
                           VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;

        VkImageViewCreateInfo viewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image,
            .viewType         = viewType,
            .format           = imageCreateInfo.format,
            .subresourceRange = {.aspectMask = imageAspect, .baseMipLevel = 0, .levelCount = imageCreateInfo.mipLevels, .baseArrayLayer = 0, .layerCount = imageCreateInfo.arrayLayers}
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
            .depth       = imageCreateInfo.extent.depth,
            .channels    = vkuFormatComponentCount(imageCreateInfo.format),
            .mipCount    = imageCreateInfo.mipLevels,
            .layers      = imageCreateInfo.arrayLayers
        }));

        GPUResourceManager::getInstance().registerResourceState<ImageState>(key, initialState);

        cache[std::type_index(typeid(Image))][name] = key;

        return std::move(ResourceHandle<Image>(key));
    }

    void ResourceManager::recreate(const ResourceHandle<Image>& handle, const VkImageCreateInfo& imageCreateInfo, const VkImageAspectFlags imageAspect, const PassKey<ResourceHandle<Image>>&) {
        auto& pool = pools.get<Image>();

        assert(handle.key().generations == pool.getSlot(handle.key()).generations);

        Image& image = pool[handle.key()];

        auto device = *Core::getInstance().getDevice();
        
        vkDestroyImageView(device, image.view, nullptr);
        
        if (image.managed) {
            GPUResourceManager::getInstance().releaseBuffer(image.image, image.allocation);
            GPUResourceManager::getInstance().uploadData(image.image, image.allocation, imageCreateInfo);
        }

        VkImageViewType viewType = imageCreateInfo.extent.depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : 
                           std::min(imageCreateInfo.extent.height, imageCreateInfo.extent.width) > 1 ? 
                           VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;

        VkImageViewCreateInfo viewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image.image,
            .viewType         = viewType,
            .format           = imageCreateInfo.format,
            .subresourceRange = {.aspectMask = imageAspect, .baseMipLevel = 0, .levelCount = imageCreateInfo.mipLevels, .baseArrayLayer = 0, .layerCount = imageCreateInfo.arrayLayers }
        };

        vkCreateImageView(*Core::getInstance().getDevice(), &viewCreateInfo, nullptr, &image.view);

        GPUResourceManager::getInstance().updateResourceState<Image, ImageState>(handle, { make_state_array({}), VK_QUEUE_FAMILY_IGNORED });
    }

    const ResourceHandle<Image> ResourceManager::registerExternal(
                const std::string& name, const VkImage& image, const VkImageAspectFlags imageAspect, const VkFormat format,
                const uint32_t width, const uint32_t height, const uint32_t depth,
                const uint32_t mipCount, const uint32_t layerCount,
                const ImageState& state,
                const VmaAllocation& allocation) {

        VkImageViewType viewType = depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : 
                           std::min(height, width) > 1 ? 
                           VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;

        VkImageViewCreateInfo viewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image,
            .viewType         = viewType,
            .format           = format,
            .subresourceRange = {.aspectMask = imageAspect, .baseMipLevel = 0, .levelCount = mipCount, .baseArrayLayer = 0, .layerCount = layerCount }
        };

        VkImageView view;
        vkCreateImageView(*Core::getInstance().getDevice(), &viewCreateInfo, nullptr, &view);

        SlotKey key = pools.get<Image>().emplace(std::move( Image{
            .name        = name,
            .allocation  = allocation,
            .image       = image,
            .view        = view,
            .aspectFlags = imageAspect,
            .width       = width,
            .height      = height,
            .depth       = depth,
            .channels    = vkuFormatComponentCount(format),
            .mipCount    = mipCount,
            .layers      = layerCount,
            .managed     = 0
        }));

        GPUResourceManager::getInstance().registerResourceState<ImageState>(key, state);

        cache[std::type_index(typeid(Image))][name] = key;

        return std::move(ResourceHandle<Image>(key));
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