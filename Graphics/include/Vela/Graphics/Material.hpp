#ifndef VELA_GRAPHICS_MATERIAL_HPP
#define VELA_GRAPHICS_MATERIAL_HPP

#include "VertexLayout.hpp"
#include "RenderTypes.hpp"
#include "DynamicBuffer.hpp"

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
    struct RenderState
    {
        CullMode cull{CullMode::None};
        FrontFace frontFace{FrontFace::Clockwise};
        bool depthTest{true};
        bool depthWrite{true};
        DepthCompare depthCompare{DepthCompare::Less};
        BlendMode blend{BlendMode::Opaque};
    };

    struct MaterialDescription
    {
        std::span<const uint32_t> vertexShader;
        std::span<const uint32_t> fragmentShader;
        VertexLayout vertexLayout;
        RenderState renderState{};
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

        Status setStorageBuffer(uint32_t slot, const DynamicBuffer& buffer);

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