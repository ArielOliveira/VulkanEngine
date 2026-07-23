#include <engine/fileParser.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <ktx.h>

#include <runtime/fileHelper.hpp>

#include <filesystem>
#include <vector>
#include <stack>

using Engine::Descriptors::TextureMetaData;

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

    const std::pair<TextureMetaData, void*> FileParser::openTextureFile(const std::string& path) {
        ktxTexture2* kTexture;
        KTX_error_code result = ktxTexture2_CreateFromNamedFile(
            path.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
            &kTexture
        );

        
        if (result != KTX_SUCCESS) 
            throw std::runtime_error("Failed to load ktx texture image! " + std::to_string(result));
        
        return { std::move(TextureMetaData {
            .width           = kTexture->baseWidth,
            .height          = kTexture->baseHeight,
            .depth           = kTexture->baseDepth,
            .format          = kTexture->vkFormat,
            .channels        = ktxTexture2_GetNumComponents(kTexture),
            .dataSize        = kTexture->dataSize,
            .generateMipmaps = kTexture->generateMipmaps,
            .data            = kTexture->pData, }), 
            (void*)kTexture 
        };

        
    }

    void FileParser::closeTextureFile(void* handle) {
        ktxTexture_Destroy((ktxTexture*)handle);
    }
}