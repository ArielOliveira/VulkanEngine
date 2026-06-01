#include <application.hpp>

Application& Application::getInstance() {
    static Application instance;

    return instance;
}

Application::Application() {
    glfwSetErrorCallback(error_callback);
    
    if (!glfwInit()) throw std::runtime_error("Failed to initialize platform!");
    
    WindowParameters parameters; // using default values;

    std::string title = std::string();
    title += parameters.title; title += ' '; title += VULKAN_ENGINE_VERSION_STRING; title += '-'; title += BUILD_TYPE;
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(parameters.width, parameters.height, title.c_str(), nullptr, nullptr);
    if (!window) throw std::runtime_error("Failed to create window!");

    glfwSetKeyCallback(window, key_callback);

    updateListeners = vector<void(*)()>();
}

Application::~Application() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::update() const {
    glfwPollEvents();
    update_callback();
}

void Application::error_callback(const int error, const char* description) { cerr << "Error " << error << ": " << description << endl; }

void Application::key_callback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Application::update_callback() const {
    for (void (*listener)() : updateListeners)
        listener();
}

void Application::getFramebufferSize(int *width, int *height) { glfwGetFramebufferSize(window, width, height); }

void Application::addUpdateListener(void (*listener)()) {
    updateListeners.push_back(listener);
}

void Application::removeUpdateListener(void (*listener)()) {
    for (auto it = updateListeners.begin(); it != updateListeners.end(); ++it) {
        if (*it == listener) {
            updateListeners.erase(it);
            return;
        }
    }
}

GLFWwindow* Application::getWindow() const { return window; }

const bool Application::shouldClose() const { return glfwWindowShouldClose(window); }

vector<const char*> Application::getRequiredExtensions(uint32_t* count) const { auto extensions = glfwGetRequiredInstanceExtensions(count); return vector(extensions, extensions+(*count)); }
const char*  Application::name() const { return glfwGetWindowTitle(window); }