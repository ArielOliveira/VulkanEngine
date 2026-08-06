#ifndef ENGINE_DESCRIPTORS_HPP
#define ENGINE_DESCRIPTORS_HPP

#include <cstdint>
#include <vector>

namespace Engine::Descriptors {
    struct ImageMipLayout {
        uint32_t width;
        uint32_t height;
        uint32_t depth;

        size_t storageRowPitch;

        size_t offset;
        size_t size;
    };

    struct ImageLayout {
        std::vector<ImageMipLayout> mips;

        uint32_t texelSize;
        uint32_t blockWidth  = 1;
        uint32_t blockHeight = 1;
        uint32_t blockDepth  = 1;
        
        uint32_t layers   = 1;
        uint32_t faces    = 1;
        uint32_t channels = 1;
        
        uint32_t format;
    };

    struct TextureAsset {
        ImageLayout layout;
        
        std::byte* data; 
        size_t dataSize;
        
        bool generateMipmaps;
    };
}

#endif