#ifndef VELA_BACKEND_SPIRV_REFLECT_HPP
#define VELA_BACKEND_SPIRV_REFLECT_HPP

#include <cstdint>
#include <span>

namespace vela::backend
{
    [[nodiscard]] uint32_t reflectTextureCount(std::span<const uint32_t> vertexShader,
        std::span<const uint32_t> fragmentShader);
} //namespace vela::backend

#endif //VELA_BACKEND_SPIRV_REFLECT_HPP
