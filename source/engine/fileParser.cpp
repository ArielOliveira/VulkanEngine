#include <engine/fileParser.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <vulkan/utility/vk_format_utils.h>

#include <ktx.h>

#include <runtime/fileHelper.hpp>

#include <filesystem>
#include <vector>
#include <stack>

using Engine::Descriptors::ImageLayout;
using Engine::Descriptors::ImageMipLayout;

namespace Engine {
    const std::tuple<Parser, Expected<Asset>, std::string>  FileParser::openModelFile(const std::string& path) {
        Parser parser;

        std::filesystem::path systemPath = path;
        
        auto data = fastgltf::GltfDataBuffer::FromPath(systemPath);

        if (data.error() != fastgltf::Error::None)
            throw std::runtime_error("Couldn't load file from path: " + systemPath.string() + " Error Code: " + std::to_string((int)data.error()));

        auto asset = parser.loadGltf(data.get(), systemPath.parent_path(), fastgltf::Options::None);

        if (auto error = asset.error(); error != fastgltf::Error::None) 
            throw std::runtime_error("Couldn't parse file from path: " + systemPath.string() + " Error Code: " + std::to_string((int)error));
        
        return { std::move(parser), std::move(asset), systemPath.filename().string() };
    }

    const std::pair<TextureAsset, void*> FileParser::openTextureFile(const std::string& path) {
        ktxTexture2* kTexture;
        KTX_error_code result = ktxTexture2_CreateFromNamedFile(
            path.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
            &kTexture
        );

        
        if (result != KTX_SUCCESS) 
            throw std::runtime_error("Failed to load ktx texture image! " + std::to_string(result));        

        auto extent = vkuFormatTexelBlockExtent((VkFormat)kTexture->vkFormat);
        
        ImageLayout layout {
            .texelSize   = static_cast<uint32_t>(ktxTexture_GetElementSize((ktxTexture*)kTexture)),
            .blockWidth  = extent.width,
            .blockHeight = extent.height,
            .blockDepth  = extent.depth,
            .layers      = static_cast<uint32_t>(kTexture->numLayers),
            .faces       = static_cast<uint32_t>(kTexture->numFaces),
            .channels    = static_cast<uint32_t>(ktxTexture2_GetNumComponents(kTexture)),
            .format      = static_cast<uint32_t>(kTexture->vkFormat),
        };

        layout.mips.reserve(static_cast<uint32_t>(kTexture->numLevels));

        uint32_t width  = kTexture->baseWidth;
        uint32_t height = kTexture->baseHeight;
        uint32_t depth  = kTexture->baseDepth;
        size_t offset = 0;
        
        assert(depth == 1 && "3D Texture currently not supported");

        for (auto i = 0; i < kTexture->numLevels; i++) {
            ktxTexture_GetImageOffset((ktxTexture*)kTexture, i, 0, 0, &offset);

            layout.mips.emplace_back(
                width, height, kTexture->baseDepth, 
                static_cast<size_t>(ktxTexture_GetRowPitch((ktxTexture*)kTexture, i)),
                static_cast<size_t>(offset),
                static_cast<size_t>(ktxTexture_GetImageSize((ktxTexture*)kTexture, i)));
            
            width  >>= 1;
            height >>= 1;
        }

        return { 
            TextureAsset {
                std::move(layout),
                reinterpret_cast<std::byte*>(kTexture->pData),
                kTexture->dataSize,
                kTexture->generateMipmaps
            },

            reinterpret_cast<void*>(kTexture)
        };
    }

    void FileParser::closeTextureFile(void* handle) {
        assert(handle != nullptr && "Invalid texture file handle!");

        ktxTexture2_Destroy(reinterpret_cast<ktxTexture2*>(handle));
    }
}