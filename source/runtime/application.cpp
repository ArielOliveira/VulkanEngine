#include <runtime/application.hpp>

namespace Runtime {
    Application& Application::getInstance() {
        static Application instance;

        return instance;
    }

    Application::Application() {
        time = glfwGetTime();

        glfwSetErrorCallback(error_callback);
        
        if (!glfwInit()) throw std::runtime_error("Failed to initialize platform!");
        
        WindowParameters parameters; // using default values;

        std::string title = std::string();
        title += parameters.title; title += ' '; title += VULKAN_ENGINE_VERSION_STRING; title += '-'; title += BUILD_TYPE;
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(parameters.width, parameters.height, title.c_str(), nullptr, nullptr);
        if (!window) throw std::runtime_error("Failed to create window!");
        windowDirty = false;
        windowState = WindowState::WINDOW_ON_FOCUS;

        glfwSetWindowUserPointer(window, this); // Do I need it?
        glfwSetKeyCallback(window, key_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

        updateListeners = vector<void(*)()>();
    }

    Application::~Application() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Application::update() {
        glfwPollEvents();
        update_callback();

        deltaTime = glfwGetTime() - time;
        time = glfwGetTime();
    }

    void Application::error_callback(const int error, const char* description) { cerr << "Error " << error << ": " << description << endl; }

    void Application::key_callback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    void Application::framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
        // Is this still needed in my case?
        // auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        
        getInstance().windowDirty = true;

        if (width <= 0 || height <= 0)
            getInstance().windowState = WindowState::WINDOW_NULL;
        else 
            getInstance().windowState = WindowState::WINDOW_ON_FOCUS;
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
    const WindowState Application::getWindowState() const { return windowState; }
}