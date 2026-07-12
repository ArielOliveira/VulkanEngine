#include <iostream>
#include <graphics/core.hpp>
#include <graphics/renderer.hpp>
#include <engine/resourceManager.hpp>
#include <version.hpp>
#include <runtime/application.hpp>
#include <engine/resources.hpp>

using Graphics::Core;
using Graphics::Renderer;

using Runtime::Application;

using Engine::ResourceManager;

using Utils::SlotMap;
using Utils::SlotKey;

using std::cout;
using std::endl;

int main(int argc, char** args) { 
    Resources::MeshLoader::loadMesh("viking_room.glb");

    /*Application& application = Application::getInstance();
    ResourceManager& rm = ResourceManager::getInstance();
    Core& core = Core::getInstance();
    Renderer& renderer = Renderer::getInstance();

    while(!application.shouldClose()) {
        application.update();
        renderer.drawFrame();
    }

    core.getDevice().waitIdle();*/

    return 0;
}