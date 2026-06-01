#ifndef FILE_HELPER_HPP
#define FILE_HELPER_HPP

#if defined(_WIN32)
#include <windows.h>
#elif define(__linux__)
#include <unistd.h>
#include <limits.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

using std::vector;
using std::ifstream;
using std::string;
using std::ios;



namespace FileHelper {
    vector<char> readFile(const string &fileName, const ios::openmode& flags);
    const std::filesystem::path getExecutablePath(bool includeFileName = false);
    const std::filesystem::path getExecutableRelativePath(bool includeFileName = false);
}

#endif