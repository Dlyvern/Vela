#ifndef VELA_GRAPHICS_MATERIAL_HPP
#define VELA_GRAPHICS_MATERIAL_HPP

#include "Vela/Graphics/VertexLayout.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>

#include "Vela/Result.hpp"

namespace vela::backend
{
    class MaterialImpl;
} //namespace vela::backend

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::graphics
{
    class Texture;
} //namespace vela::graphics

namespace vela::graphics
{
    struct MaterialDescription
    {
        std::span<const uint32_t> vertexShader;
        std::span<const uint32_t> fragmentShader;
        VertexLayout vertexLayout;
        uint32_t textureCount{0};
    };

    class Material
    {
    public:
        static Result<Material> create(core::Context& ctx, const MaterialDescription& description);
        ~Material();

        Material(Material&&) noexcept;
        Material& operator=(Material&&) noexcept;

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;

        Status setTexture(uint32_t slot, const Texture& texture);

        template<typename Slot> requires std::is_enum_v<Slot>
        Status setTexture(Slot slot, const Texture& texture)
        {
            return setTexture(static_cast<uint32_t>(slot), texture);
        }

        backend::MaterialImpl* impl() const;

        const MaterialDescription& getMaterialDescription() const;
    private:
        Material(core::Context& ctx, const MaterialDescription& description);

        std::unique_ptr<backend::MaterialImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_MATERIAL_HPP