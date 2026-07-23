#ifndef ENGINE_FILE_PARSER_HPP
#define ENGINE_FILE_PARSER_HPP

#include <string>

#include <fastgltf/core.hpp>
using fastgltf::Parser;
using fastgltf::Expected;
using fastgltf::Asset;

#include <filesystem>

namespace Engine::Descriptors {
    struct TextureMetaData {
        uint32_t width;
        uint32_t height;
        uint32_t depth;

        uint32_t format;
        uint32_t channels;

        size_t dataSize;
        
        bool generateMipmaps;

        void* data; 
    };
}

namespace Engine::Paths {
    const std::filesystem::path MODELS            = "../../models/";
    const std::filesystem::path TEXTURES          = "../../textures/";
    const std::filesystem::path SHADERS           = "./spirv/";
}

namespace Engine::FileParser {
    using Descriptors::TextureMetaData;

    const std::tuple<Parser, Expected<Asset>, std::string>  openModelFile(const std::string& path);
    const std::pair<TextureMetaData, void*> openTextureFile(const std::string& path);
    void closeTextureFile(void* handle);
}

#endif