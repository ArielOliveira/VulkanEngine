#include <iostream>
#include <graphics/core.hpp>
#include <graphics/renderer.hpp>
#include <version.hpp>
#include <application/application.hpp>
#include <fileHelper.hpp>

using Graphics::Core;
using Graphics::Renderer;

using std::cout;
using std::endl;

int main(int argc, char** args) {    
    Application& application = Application::getInstance();
    Core& core = Core::getInstance();
    Renderer& renderer = Renderer::getInstance();

    while(!application.shouldClose()) {
        application.update();
        //graphics.drawFrame();
    }

    core.getDevice().waitIdle();

    return 0;
}