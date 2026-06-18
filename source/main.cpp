#include <iostream>
#include <graphics/core.hpp>
#include <version.hpp>
#include <application.hpp>
#include <fileHelper.hpp>

using Graphics::Core;

using std::cout;
using std::endl;

int main(int argc, char** args) {    
    Application& application = Application::getInstance();
    Core& core = Core::getInstance();

    while(!application.shouldClose()) {
        application.update();
        //graphics.drawFrame();
    }

    core.getDevice().waitIdle();

    return 0;
}