#ifndef ENGINE_RESOURCE_MANAGER_TPP
#define ENGINE_RESOURCE_MANAGER_TPP

#include <engine/resourceManager.hpp>

namespace Engine {
    template<>
    inline const ResourceHandle<Mesh> ResourceManager::load<Mesh>(const std::string& sourceName, const Asset& asset, const size_t resourceIndex) {
        auto& meshR = asset.meshes[resourceIndex];
        std::string resourceName = sourceName + "#" + std::string(meshR.name.c_str());
        
        assert(findKeyByName(std::type_index(typeid(Mesh)), resourceName) == nullptr);

        std::cout << "Loading mesh " << resourceName << '\n';
        
        // Setup mesh data
        Mesh mesh = {
            .name           = resourceName,
            .smIndexOffset  = std::vector<uint32_t>(),
            .smIndexCount   = std::vector<uint32_t>(),
            .smVertexOffset = std::vector<uint32_t>(),
            .smMaterials    = std::vector<ResourceHandle<Material>>()
        };

        mesh.smIndexOffset.reserve(meshR.primitives.size());
        mesh.smIndexCount.reserve(meshR.primitives.size());
        mesh.smVertexOffset.reserve(meshR.primitives.size());
        mesh.smMaterials.reserve(meshR.primitives.size());
        
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

        mesh.vertexCount = vertexCount;
        mesh.indexCount  = indexCount;
        mesh.indexType   = vk::IndexType::eUint32;
        
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
                
                if (uv0It)
                    uv0   = fastgltf::getAccessorElement<glm::vec2>(asset, *uv0Accessor, vertex);

                vertices.emplace_back(pos, color, uv0);
            }

            const auto* indicesAccessor = primitive.indicesAccessor.has_value() ? &asset.accessors[primitive.indicesAccessor.value()] : nullptr;
            
            if (indicesAccessor != nullptr) {
                fastgltf::iterateAccessor<uint32_t>(asset, *indicesAccessor, [&](uint32_t index) {
                    indices.push_back(index);
                });
                
                mesh.smIndexCount.push_back(static_cast<uint32_t>(indicesAccessor->count));
                mesh.smIndexOffset.push_back(static_cast<uint32_t>(indexOffset));
                indexOffset  += indices.size();
            }

            mesh.smVertexOffset.push_back(static_cast<uint32_t>(vertexOffset));
            
            vertexOffset += vertices.size();
        }

        std::cout << "Mesh " << resourceName << " has " << vertices.size() << "|" << mesh.vertexCount << " vertices" << '\n';

        GPUMemoryManager::getInstance().uploadDataToGPU(
                        mesh.vertexBuffer, mesh.vertexAllocation, 
                        vertices.data(), sizeof(vertices[0]) * vertices.size(), 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        
        if (indices.size() > 0) {
            GPUMemoryManager::getInstance().uploadDataToGPU(
                            mesh.indexBuffer, mesh.indexAllocation, 
                            indices.data(), sizeof(indices[0]) * indices.size(), 
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        }

        std::cout << "Registering mesh " << resourceName.c_str() << '\n';
        SlotKey key = pools.get<Mesh>().insert(mesh);
        
        cache[std::type_index(typeid(Mesh))][resourceName] = key;

        return std::move(ResourceHandle<Mesh>(key));
    }

    template<>
    inline const ResourceHandle<Scene> ResourceManager::load<Scene>(const std::string& sourceName, const Asset& asset, const size_t resourceIndex) {
        auto& sceneStr = asset.scenes[resourceIndex];
        std::string resourceName =  std::string(sourceName.c_str()) + "#" + std::string(sceneStr.name.c_str());

        std::cout << "Loading scene: " << resourceName << '\n';

        assert(findKeyByName(std::type_index(typeid(Scene)), resourceName) == nullptr);

        size_t nodeCount = sceneStr.nodeIndices.size();
        
        Scene scene = {
            .name       = resourceName,
            .nodeNames  = std::vector<std::string>(),
            .hierarchy  = std::vector<uint32_t>(),
            .transforms = std::vector<glm::mat4x4>(),
            .meshes     = std::vector<std::pair<ResourceHandle<Mesh>, uint32_t>>(),
            .animations = std::vector<std::pair<ResourceHandle<Animation>, uint32_t>>()
        };

        scene.nodeNames.reserve(nodeCount);
        scene.hierarchy.reserve(nodeCount);
        scene.transforms.reserve(nodeCount);
        
        std::stack<std::tuple<size_t, std::optional<size_t>, uint32_t>> nodeStack;
            
        for (auto nodeIndex : sceneStr.nodeIndices | std::views::reverse)
            nodeStack.push({ nodeIndex, {}, 0 });

        while(!nodeStack.empty()) {
            auto [current, parent, hierarchyLayer]  = nodeStack.top();
            auto& node                              = asset.nodes[current];
            nodeStack.pop();

            scene.nodeNames.emplace_back(node.name.c_str());
            scene.hierarchy.push_back(hierarchyLayer-1);
            scene.transforms.emplace_back(glm::make_mat4x4(fastgltf::getTransformMatrix(node).data()));

            if (node.meshIndex.has_value()) {
                std::string meshName = sourceName + "#" + std::string(asset.meshes[node.meshIndex.value()].name.c_str());
                
                if (SlotKey* key = findKeyByName(std::type_index(typeid(Mesh)), meshName); key == nullptr) {
                    scene.meshes.emplace_back(load<Mesh>(sourceName, asset, node.meshIndex.value()), static_cast<uint32_t>(scene.hierarchy.size())-1);
                } else {
                    scene.meshes.emplace_back(*key, static_cast<uint32_t>(scene.hierarchy.size())-1);
                }
            }
                
            for (auto childIndex : node.children | std::views::reverse)
                nodeStack.push({ childIndex, current, hierarchyLayer+1 });
        }

        std::cout << "Registering scene " << resourceName.c_str() << '\n';
        SlotKey key = pools.get<Scene>().insert(scene);
        
        cache[std::type_index(typeid(Scene))][resourceName] = key;

        return std::move(ResourceHandle<Scene>(key));
    }

    inline const ResourceHandle<Model> ResourceManager::load(const std::string& path) {
        assert(findKeyByName(std::type_index(typeid(Model)), path) == nullptr);

        std::cout << "Loading model: " << path << '\n';
    
        auto [parser, asset, fileName] = FileParser::openFile(path);

        Model model = {
            .name       = fileName.c_str(),
            .scenes     = std::vector<ResourceHandle<Scene>>()
        };

        model.scenes.reserve(asset.get().scenes.size());
        
        for (size_t sceneIndex = 0; sceneIndex < asset.get().scenes.size(); sceneIndex++) {
            model.scenes.emplace_back(load<Scene>(fileName, asset.get(), sceneIndex));
        }

        std::cout << "Registering model " << path << '\n';
        SlotKey key = pools.get<Model>().insert(model);
        
        cache[std::type_index(typeid(Model))][model.name] = key;

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
            std::cout << "Destroying " << pool[key].name << '\n';
            cache[std::type_index(typeid(Resource))].erase(pool[key].name);
            pool.erase(key);
        }
    }

    template <>
    inline void ResourceManager::release<Mesh>(const SlotKey& key, const PassKey<ResourceHandle<Mesh>>&) {
        auto& pool = pools.get<Mesh>();
        
        assert(key.generations == pool.getSlot(key).generations);
        
        if (--pool.getSlot(key).refCount == 0) {
            std::cout << "Freeing Mesh GPU memory for " << pool[key].name << '\n';
            
            Mesh m = pool[key];

            GPUMemoryManager::getInstance().releaseBuffer(m.vertexBuffer, m.vertexAllocation);
            GPUMemoryManager::getInstance().releaseBuffer(m.indexBuffer, m.indexAllocation);

            std::cout << "Destroying mesh " <<m.name << '\n';
            cache[std::type_index(typeid(Mesh))].erase(m.name);
            pool.erase(key);
        }
    }

    template <typename Resource>
    const Resource& ResourceManager::get(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const {
        auto& pool = pools.get<Resource>();

        assert(key.generations == pool.getSlot(key).generations);

        return pool[key];
    }
}

#endif