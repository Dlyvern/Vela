#ifndef VELA_ASSETS_RESOURCES_HPP
#define VELA_ASSETS_RESOURCES_HPP

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

#include <filesystem>
#include <string>
#include <stdexcept>
#include <cstdint>

namespace vela::assets::resources
{
    // Absolute path of the directory containing the running executable.
    inline std::filesystem::path executableDirectory()
    {
    #if defined(_WIN32)
        wchar_t buf[MAX_PATH];
        GetModuleFileNameW(nullptr, buf, MAX_PATH);
        return std::filesystem::path(buf).parent_path();
    #elif defined(__APPLE__)
        char buf[1024];
        uint32_t size = sizeof(buf);
        if (_NSGetExecutablePath(buf, &size) != 0)
            throw std::runtime_error("Executable path buffer too small");
        return std::filesystem::canonical(buf).parent_path();
    #else
        return std::filesystem::canonical("/proc/self/exe").parent_path();
    #endif
    }

    // Resolve a path relative to the executable directory.
    // Example: find("shaders/my_shader.vert.spv")
    inline std::filesystem::path find(const std::string& relativePath)
    {
        return executableDirectory() / relativePath;
    }
}

#endif //VELA_ASSETS_RESOURCES_HPP
