#ifndef ENGINE_RESOURCE_MANAGER_TPP
#define ENGINE_RESOURCE_MANAGER_TPP

#include <engine/resourceManager.hpp>

namespace Engine {
    template<>
    const ResourceHandle<Mesh> ResourceManager::load<Mesh>(const std::string& sourceName, const Asset& asset, const size_t resourceIndex) {
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
        
        size_t vertexCount  = 0;
        size_t indexCount = 0;
        for (auto& primitive : meshR.primitives) {
            auto* posIt  = primitive.findAttribute("POSITION");
            
            assert(posIt != nullptr);
            assert(primitive.indicesAccessor.has_value());

            auto& posAccessor     = asset.accessors[posIt->accessorIndex];
            auto& indicesAccessor = asset.accessors[primitive.indicesAccessor.value()];

            vertexCount          += posAccessor.count;
            indexCount           += indicesAccessor.count;
        }

        mesh.vertexCount = vertexCount;
        mesh.indexCount  = indexCount;
        mesh.indexType   = VK_INDEX_TYPE_UINT32;
        
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
                glm::vec3 color;
                glm::vec2 uv0;

                if (colorIt)
                    color = fastgltf::getAccessorElement<glm::vec3>(asset, *colorAccessor, vertex);
                
                if (uv0It)
                    uv0   = fastgltf::getAccessorElement<glm::vec2>(asset, *uv0Accessor, vertex);

                vertices.emplace_back(pos, color, uv0);
            }

            auto& indicesAccessor = asset.accessors[primitive.indicesAccessor.value()];

            fastgltf::iterateAccessor<uint32_t>(asset, indicesAccessor, [&](uint32_t index) {
                indices.push_back(index);
            });

            mesh.smVertexOffset.push_back(static_cast<uint32_t>(vertexOffset));
            mesh.smIndexOffset.push_back(static_cast<uint32_t>(indexOffset));
            mesh.smIndexCount.push_back(static_cast<uint32_t>(indicesAccessor.count));

            vertexOffset += vertices.size();
            indexOffset  += indices.size();
        }

        GPUMemoryManager::getInstance().uploadDataToGPU(
                        mesh.vertexBuffer, mesh.vertexAllocation, 
                        vertices.data(), vertices.size(), 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        GPUMemoryManager::getInstance().uploadDataToGPU(
                        mesh.indexBuffer, mesh.indexAllocation, 
                        indices.data(), indices.size(), 
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        std::cout << "Registering mesh " << resourceName.c_str() << '\n';
        SlotKey key = pools.get<Mesh>().insert(mesh);
        
        cache[std::type_index(typeid(Mesh))][resourceName] = key;

        return std::move(ResourceHandle<Mesh>(key));
    }

    template<>
    const ResourceHandle<Scene> ResourceManager::load<Scene>(const std::string& sourceName, const Asset& asset, const size_t resourceIndex) {
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
    void ResourceManager::release<Mesh>(const SlotKey& key, const PassKey<ResourceHandle<Mesh>>&) {
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

    template <typename T>
    ResourceHandle<T>::ResourceHandle(std::nullptr_t) noexcept : key({~0U, ~0U}) {}

    template <typename T>
    ResourceHandle<T>::ResourceHandle(SlotKey key) noexcept : key(key) {
        ResourceManager::getInstance().acquire<T>(key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(const ResourceHandle& rhs) noexcept : key(rhs.key) {
        ResourceManager::getInstance().acquire<T>(key, {});
    }

    template <typename T>
    ResourceHandle<T>::ResourceHandle(ResourceHandle&& rhs) noexcept { std::swap(key, rhs.key); }

    template <typename T>
    ResourceHandle<T>::~ResourceHandle() noexcept {
        if (key.index != ~0U)
        ResourceManager::getInstance().release<T>(key, {});
    }

    template <typename T>
    const ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t) noexcept {
        key = { ~0U, ~0U };
    }

    template <typename T>
    ResourceHandle<T>& ResourceHandle<T>::operator=(ResourceHandle&& rhs) noexcept {
        if (this != &rhs) 
            std::swap(key, rhs.key);
        
        rhs.key = { ~0U, ~0U };

        return *this;
    }

    template <typename T>
    const T& ResourceHandle<T>::data() const {
        return ResourceManager::getInstance().get<T>(key, {});
    }
}

#endif