#include "Vela/Assets/Shader.hpp"

#include <fstream>

namespace vela::assets
{
    Result<std::vector<uint32_t>> loadSpirv(const std::string& path)
    {
        constexpr uint32_t k_spirvMagic = 0x07230203;

        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            return Error{ErrorCode::ShaderLoadFailed, "Failed to open " + path};

        const std::streamsize size = file.tellg();

        if (size <= 0 || size % static_cast<std::streamsize>(sizeof(uint32_t)) != 0)
            return Error{ErrorCode::ShaderLoadFailed, path + " is not SPIR-V: size is not a multiple of 4"};

        std::vector<uint32_t> code(static_cast<size_t>(size) / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(code.data()), size);

        if (!file)
            return Error{ErrorCode::ShaderLoadFailed, "Failed to read " + path};

        if (code[0] != k_spirvMagic)
            return Error{ErrorCode::ShaderLoadFailed, path + " is not SPIR-V: bad magic number"};

        return std::move(code);
    }
} //namespace vela::assets
