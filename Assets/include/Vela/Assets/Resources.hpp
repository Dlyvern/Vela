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
#include <system_error>
#include <cstdint>

namespace vela::assets::resources
{
    // Absolute path of the directory containing the running executable.
    // Never throws.
    // Returns: the directory, or an empty path if the platform could not report the executable's location.
    inline std::filesystem::path executableDirectory() noexcept
    {
        std::error_code error;

    #if defined(_WIN32)
        std::wstring buffer(MAX_PATH, L'\0');

        while (true)
        {
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

            if (length == 0)
                return {};

            if (length < buffer.size())
            {
                buffer.resize(length);
                break;
            }

            buffer.resize(buffer.size() * 2);
        }

        const std::filesystem::path executable = std::filesystem::path(buffer);
    #elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);

        std::string buffer(size, '\0');

        if (_NSGetExecutablePath(buffer.data(), &size) != 0)
            return {};

        const std::filesystem::path executable = std::filesystem::canonical(buffer.c_str(), error);
    #else
        const std::filesystem::path executable = std::filesystem::canonical("/proc/self/exe", error);
    #endif

        if (error || executable.empty())
            return {};

        return executable.parent_path();
    }

    // Resolves a path relative to the executable directory. Never throws.
    // relativePath: path below the executable directory, e.g. "shaders/my_shader.vert.spv".
    // Returns:      the absolute path, or an empty path if executableDirectory() failed.
    inline std::filesystem::path find(const std::string& relativePath) noexcept
    {
        const std::filesystem::path directory = executableDirectory();

        if (directory.empty())
            return {};

        return directory / relativePath;
    }
}

#endif //VELA_ASSETS_RESOURCES_HPP
