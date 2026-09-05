#ifndef ENGINE_RESOURCE_MANAGER_TPP
#define ENGINE_RESOURCE_MANAGER_TPP

#include <engine/resourceManager.hpp>

#include <graphics/core.hpp>
#include <graphics/models.hpp>

using Graphics::Core;

namespace Engine {
    template<>
    inline ResourceHandle<Texture> ResourceManager::load<Texture>(const std::string& path) {
        assert(findKeyByName(std::type_index(typeid(Texture)), path) == nullptr);

        std::cout << "Loading texture " << path << '\n';

        auto [textureAsset, fileHandle] = FileParser::openTextureFile(path);

        switch (textureAsset.layout.channels) {
            case 3:
            case 4:  textureAsset.layout.format = (uint32_t)vk::Format::eR8G8B8A8Srgb; break;
        }

        VkImageType imageType = textureAsset.layout.mips[0].depth > 1 ? VK_IMAGE_TYPE_3D : 
                           std::min(textureAsset.layout.mips[0].height, textureAsset.layout.mips[0].width) > 1 ? 
                           VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;

        VkImageCreateInfo imageCreateInfo {
            .sType         =  VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType     = imageType,
            .format        = (VkFormat)textureAsset.layout.format,
            .extent        = { textureAsset.layout.mips[0].width, textureAsset.layout.mips[0].height, textureAsset.layout.mips[0].depth },
            .mipLevels     = static_cast<uint32_t>(textureAsset.layout.mips.size()),
            .arrayLayers   = textureAsset.layout.layers,
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            .tiling        = VK_IMAGE_TILING_OPTIMAL,
            .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE
        };
      
        ResourceHandle<Image> imageHandle = GPUResourceManager::getInstance().create(
            textureAsset.data, textureAsset.dataSize, textureAsset.layout, 
            imageCreateInfo, VK_IMAGE_ASPECT_COLOR_BIT);
        
        vk::PhysicalDeviceProperties properties = Core::getInstance().getPhysicalDevice().getProperties();

        VkSamplerCreateInfo samplerInfo {
            .sType                      = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter                  = VkFilter::VK_FILTER_LINEAR,
            .minFilter                  = VkFilter::VK_FILTER_LINEAR,
            .mipmapMode                 = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU               = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV               = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW               = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias                 = 0.0f,
            .anisotropyEnable           = vk::True,
            .maxAnisotropy              = properties.limits.maxSamplerAnisotropy,
            .compareEnable              = vk::False,
            .compareOp                  = VkCompareOp::VK_COMPARE_OP_ALWAYS,
            .minLod                     = 0,
            .maxLod                     = vk::LodClampNone,
            .borderColor                = VkBorderColor::VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates    = vk::False
        };

        VkSampler sampler;
        vkCreateSampler(*Core::getInstance().getDevice(), &samplerInfo, nullptr, &sampler);

        FileParser::closeTextureFile(fileHandle);

        std::cout << "Registering texture " << path.c_str() << '\n';
        std::string resourceName = path;
        
        SlotKey key = pools.get<Texture>().emplace(Texture {
            std::move(std::string(resourceName)),
            std::move(imageHandle),
            std::move(sampler)
        });
        
        cache[std::type_index(typeid(Texture))][resourceName] = key;

        return std::move(ResourceHandle<Texture>(key));
    }

    template<>
    inline ResourceHandle<Texture> ResourceManager::load<Texture>(const std::string& sourceName, const Asset& asset, const size_t resourceIndex) {
        return nullptr;
    }

    template<>
    inline ResourceHandle<Mesh> ResourceManager::load<Mesh>(const std::string& sourceName, const Asset& asset, const size_t resourceIndex) {
        auto& meshR = asset.meshes[resourceIndex];
        std::string resourceName = sourceName + "#" + std::string(meshR.name.c_str());
        
        assert(findKeyByName(std::type_index(typeid(Mesh)), resourceName) == nullptr);

        std::cout << "Loading mesh " << resourceName << '\n';
        
        std::vector<uint32_t>                 indicesOffset;
        std::vector<uint32_t>                 indicesCount;
        std::vector<uint32_t>                 verticesOffset;
        std::vector<ResourceHandle<Material>> materials;
        
        indicesOffset.reserve(meshR.primitives.size());
        indicesCount.reserve(meshR.primitives.size());
        verticesOffset.reserve(meshR.primitives.size());
        materials.reserve(meshR.primitives.size());
        
        size_t vertexCount  = 0;
        size_t indexCount   = 0;
        for (auto& primitive : meshR.primitives) {
            auto* posIt  = primitive.findAttribute("POSITION");
            
            assert(posIt != nullptr);

            auto& posAccessor                         = asset.accessors[posIt->accessorIndex];
            const fastgltf::Accessor* indicesAccessor = nullptr;

            if (primitive.indicesAccessor.has_value())
                indicesAccessor = &asset.accessors[primitive.indicesAccessor.value()];

            vertexCount          += posAccessor.count;

            if (indicesAccessor)
                indexCount += indicesAccessor->count;
        }
        
        std::vector<Graphics::Models::Vertex> vertices;
        std::vector<uint32_t> indices;

        vertices.reserve(vertexCount);
        indices.reserve(indexCount);

        size_t vertexOffset   = 0; 
        size_t indexOffset    = 0;

        // Begin processing data
        for (auto& primitive : meshR.primitives) {
            auto* posIt    = primitive.findAttribute("POSITION");
            auto* colorIt  = primitive.findAttribute("COLOR");
            auto* uv0It    = primitive.findAttribute("TEXCOORD_0");
            assert(posIt != nullptr);

            auto& posAccessor    = asset.accessors[posIt->accessorIndex];
            auto* colorAccessor  = colorIt  != nullptr ? &asset.accessors[colorIt->accessorIndex] : nullptr;
            auto* uv0Accessor    = uv0It    != nullptr ? &asset.accessors[uv0It->accessorIndex]    : nullptr;
            
            if (!colorIt)
                std::cout << "No color attribute found for " << resourceName << '\n';
            
            if (!uv0It)
                std::cout << "No texture coordinates found for " << resourceName << '\n';
            
            for (size_t vertex = 0; vertex < posAccessor.count; vertex++) {
                glm::vec3 pos   = fastgltf::getAccessorElement<glm::vec3>(asset, posAccessor, vertex);
                pos.y *= -1;

                glm::vec3 color;
                glm::vec2 uv0;

                if (colorIt)
                    color = fastgltf::getAccessorElement<glm::vec3>(asset, *colorAccessor, vertex);
                
                if (uv0It) {
                    uv0   = fastgltf::getAccessorElement<glm::vec2>(asset, *uv0Accessor, vertex);
                    uv0.y *= -1;
                }

                vertices.emplace_back(pos, color, uv0);
            }

            const auto* indicesAccessor = primitive.indicesAccessor.has_value() ? &asset.accessors[primitive.indicesAccessor.value()] : nullptr;
            
            if (indicesAccessor != nullptr) {
                fastgltf::iterateAccessor<uint32_t>(asset, *indicesAccessor, [&](uint32_t index) {
                    indices.push_back(index);
                });
                
                indicesCount.push_back(static_cast<uint32_t>(indicesAccessor->count));
                indicesOffset.push_back(static_cast<uint32_t>(indexOffset));
                indexOffset  += indices.size();
            }

            verticesOffset.push_back(static_cast<uint32_t>(vertexOffset));
            
            vertexOffset += vertices.size();
        }

        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize indexBufferSize  = sizeof(indices[0])  * indices.size();

        std::cout << "Mesh " << resourceName << " has " << vertices.size() << "|" << vertexCount << " vertices" << '\n';

        VkBufferCreateInfo createInfo {
            .sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size         = vertexBufferSize,
            .usage        = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode  = VK_SHARING_MODE_EXCLUSIVE
        };

        auto vertexHandle = GPUResourceManager::getInstance().create( 
                        reinterpret_cast<std::byte*>(vertices.data()),
                        createInfo);

        ResourceHandle<GPU::Buffer> indexHandle = nullptr;
        
        if (indices.size() > 0) {
            createInfo.size  = indexBufferSize;
            createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        auto indexHandle = GPUResourceManager::getInstance().create(
                            reinterpret_cast<std::byte*>(indices.data()), 
                            createInfo);
        }

        std::cout << "Registering mesh " << resourceName.c_str() << '\n';
        SlotKey key = pools.get<Mesh>().emplace(Mesh {
            std::move(std::string(resourceName)),
            std::move(vertexHandle), 
            std::move(indexHandle),
            vk::IndexType::eUint32,
            static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(indexCount),
            std::move(indicesOffset), std::move(indicesCount),
            std::move(verticesOffset),
            std::move(materials)
        });
        
        cache[std::type_index(typeid(Mesh))][resourceName] = key;

        return std::move(ResourceHandle<Mesh>(key));
    }

    template<>
    inline ResourceHandle<Scene> ResourceManager::load<Scene>(const std::string& sourceName, const Asset& asset, const size_t resourceIndex) {
        auto& sceneStr = asset.scenes[resourceIndex];
        std::string resourceName =  std::string(sourceName.c_str()) + "#" + std::string(sceneStr.name.c_str());

        std::cout << "Loading scene: " << resourceName << '\n';

        assert(findKeyByName(std::type_index(typeid(Scene)), resourceName) == nullptr);

        size_t nodeCount = sceneStr.nodeIndices.size();
        
        std::vector<std::string>                                    nodeNames;
        std::vector<uint32_t>                                       hierarchy;
        std::vector<glm::mat4x4>                                    transforms;
        std::vector<std::pair<ResourceHandle<Mesh>, uint32_t>>      meshes;
        std::vector<std::pair<ResourceHandle<Animation>, uint32_t>> animations;

        nodeNames.reserve(nodeCount);
        hierarchy.reserve(nodeCount);
        transforms.reserve(nodeCount);
        
        std::stack<std::tuple<size_t, std::optional<size_t>, uint32_t>> nodeStack;
            
        for (auto nodeIndex : sceneStr.nodeIndices | std::views::reverse)
            nodeStack.push({ nodeIndex, {}, 0 });

        while(!nodeStack.empty()) {
            auto [current, parent, hierarchyLayer]  = nodeStack.top();
            auto& node                              = asset.nodes[current];
            nodeStack.pop();

            nodeNames.emplace_back(node.name.c_str());
            hierarchy.push_back(hierarchyLayer-1);
            transforms.emplace_back(glm::make_mat4x4(fastgltf::getTransformMatrix(node).data()));

            if (node.meshIndex.has_value()) {
                std::string meshName = sourceName + "#" + std::string(asset.meshes[node.meshIndex.value()].name.c_str());
                
                if (SlotKey* key = findKeyByName(std::type_index(typeid(Mesh)), meshName); key == nullptr) {
                    meshes.emplace_back(std::move(load<Mesh>(sourceName, asset, node.meshIndex.value())), std::move(static_cast<uint32_t>(hierarchy.size())-1));
                } else {
                    meshes.emplace_back(*key, static_cast<uint32_t>(hierarchy.size())-1);
                }
            }
                
            for (auto childIndex : node.children | std::views::reverse)
                nodeStack.push({ childIndex, current, hierarchyLayer+1 });
        }

        std::cout << "Registering scene " << resourceName.c_str() << '\n';
        SlotKey key = pools.get<Scene>().emplace(Scene {
            std::move(std::string(resourceName)),
            std::move(nodeNames), std::move(hierarchy), std::move(transforms),
            std::move(meshes), std::move(animations)
        });
        
        cache[std::type_index(typeid(Scene))][resourceName] = key;

        return std::move(ResourceHandle<Scene>(key));
    }

    template<>
    inline ResourceHandle<Model> ResourceManager::load<Model>(const std::string& path) {
        assert(findKeyByName(std::type_index(typeid(Model)), path) == nullptr);

        std::cout << "Loading model: " << path << '\n';
    
        auto [parser, asset, fileName] = FileParser::openModelFile(path);

        std::string resourceName = fileName.c_str();        
        std::vector<ResourceHandle<Scene>> scenes;

        scenes.reserve(asset.get().scenes.size());
        
        for (size_t sceneIndex = 0; sceneIndex < asset.get().scenes.size(); sceneIndex++) 
            scenes.emplace_back(std::move(load<Scene>(resourceName, asset.get(), sceneIndex)));

        std::cout << "Registering model " << path << '\n';
        SlotKey key = pools.get<Model>().emplace(Model {
            std::move(std::string(resourceName)),
            std::move(scenes)
        });
        
        cache[std::type_index(typeid(Model))][resourceName] = key;

        return std::move(ResourceHandle<Model>(key));
    }

    template <typename Resource>
    void ResourceManager::acquire(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) {
        assert(key.generations == pools.get<Resource>().getSlot(key).generations);

        pools.get<Resource>().getSlot(key).refCount++;
    }

    template <typename Resource>
    void ResourceManager::release(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) {
        auto& pool = pools.get<Resource>();

        assert(key.generations == pool.getSlot(key).generations);

        if (--pool.getSlot(key).refCount == 0) {
            pool.getSlot(key).state = ResourceState::Releasing;

            std::cout << "Destroying " << pool[key].name << '\n';
            cache[std::type_index(typeid(Resource))].erase(pool[key].name);
            pool.erase(key);
        }
    }

    template <>
    inline void ResourceManager::release<Texture>(const SlotKey& key, const PassKey<ResourceHandle<Texture>>&) {
        auto& pool = pools.get<Texture>();
        
        assert(key.generations == pool.getSlot(key).generations);
        
        if (--pool.getSlot(key).refCount == 0) {
            pool.getSlot(key).state = ResourceState::Releasing;

            std::cout << "Freeing Texture " << pool[key].name << '\n';
            
            Texture& texture = pool[key];
            auto device = *Core::getInstance().getDevice();
            
            vkDestroySampler(device, texture.sampler, nullptr);

            std::cout << "Destroying Texture " << texture.name << '\n';
            cache[std::type_index(typeid(Texture))].erase(texture.name);
            pool.erase(key);
        }
    }

    template <>
    inline void ResourceManager::release<Mesh>(const SlotKey& key, const PassKey<ResourceHandle<Mesh>>&) {
        auto& pool = pools.get<Mesh>();
        
        assert(key.generations == pool.getSlot(key).generations);
        
        if (--pool.getSlot(key).refCount == 0) {
            pool.getSlot(key).state = ResourceState::Releasing;
            
            Mesh& m = pool[key];

            std::cout << "Destroying mesh " <<m.name << '\n';
            cache[std::type_index(typeid(Mesh))].erase(m.name);
            pool.erase(key);
        }
    }

    template <typename Resource>
    const bool ResourceManager::contains(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const {
        return pools.get<Resource>().contains(key);
    }

    template <typename Resource>
    const Resource& ResourceManager::get(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const {
        auto& pool = pools.get<Resource>();

        assert(key.generations == pool.getSlot(key).generations);

        return pool[key];
    }

    template <typename Resource>
    const ResourceState& ResourceManager::getState(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const {
        auto& pool = pools.get<Resource>();

        assert(key.generations == pool.getSlot(key).generations);

        return pool.getSlot(key).state;
    }
}

#endif