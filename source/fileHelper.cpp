#include <fileHelper.hpp>

vector<char> FileHelper::readFile(const string& fileName, const ios::openmode& flags) {
    ifstream file(fileName, flags);

    if (!file.is_open()) 
        throw std::runtime_error("failed to open file: " + fileName);

    vector<char> buffer(file.tellg());
        
    file.seekg(0, ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}

const std::filesystem::path FileHelper::getExecutablePath(bool includeFileName) {
    std::filesystem::path path = {};

#if defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    path = std::filesystem::path(buffer);
#elif defined(__linux__)
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
    if (len != -1) {
        buffer[len] = '\0';
        path = std::filesystem::path(buffer);
    }
#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) 
        path = std::filesystem::path(buffer);
#endif
    
    if (!includeFileName && path.has_filename())
        path.remove_filename();
    
    return path;
}

const std::filesystem::path FileHelper::getExecutableRelativePath(bool includeFileName) {
    return std::filesystem::relative(getExecutablePath(includeFileName), std::filesystem::current_path());
}