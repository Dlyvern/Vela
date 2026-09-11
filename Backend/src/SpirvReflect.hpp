#ifndef VELA_BACKEND_SPIRV_REFLECT_HPP
#define VELA_BACKEND_SPIRV_REFLECT_HPP

#include <cstdint>
#include <span>
#include <vector>

namespace vela::backend
{
    enum class DescriptorKind : uint8_t
    {
        CombinedImageSampler = 0,
        UniformBuffer,
        StorageBuffer
    };

    struct ReflectedBinding
    {
        uint32_t set{0};
        uint32_t binding{0};
        DescriptorKind kind{DescriptorKind::CombinedImageSampler};
    };

    [[nodiscard]] std::vector<ReflectedBinding> reflectBindings(std::span<const uint32_t> shader);

    [[nodiscard]] uint32_t reflectTextureCount(std::span<const uint32_t> vertexShader,
        std::span<const uint32_t> fragmentShader);

    [[nodiscard]] uint32_t reflectPushConstantSize(std::span<const uint32_t> shader);
} //namespace vela::backend

#endif //VELA_BACKEND_SPIRV_REFLECT_HPP
