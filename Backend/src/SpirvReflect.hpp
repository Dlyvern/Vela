#ifndef VELA_BACKEND_SPIRV_REFLECT_HPP
#define VELA_BACKEND_SPIRV_REFLECT_HPP

#include <cstdint>
#include <span>

namespace vela::backend
{
    [[nodiscard]] uint32_t reflectTextureCount(std::span<const uint32_t> vertexShader,
        std::span<const uint32_t> fragmentShader);

    [[nodiscard]] uint32_t reflectPushConstantSize(std::span<const uint32_t> shader);
} //namespace vela::backend

#endif //VELA_BACKEND_SPIRV_REFLECT_HPP
