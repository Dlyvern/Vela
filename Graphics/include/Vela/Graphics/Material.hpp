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

        /*
            Depth bias applied to every fragment before the depth test and write, as in vkCmdSetDepthBias:
            bias = depthBiasConstant * r + depthBiasSlope * max depth slope, where r is the smallest
            resolvable depth difference. The usual cure for shadow acne in shadow-map passes.
            Both 0 (the default) disables it
        */
        float depthBiasConstant{0.0f};
        float depthBiasSlope{0.0f};
    };

    /*
        Everything a material is built from. Material::create() copies the shader code, so the spans only need
        to stay valid during that call. getMaterialDescription() returns spans into the material's own copies.
        Shader resources are found by SPIR-V reflection: set 0 is Vela's camera, set 1 the material's textures
        and storage buffers, set 2 the pass inputs. Push constants: the two stages' blocks are merged into one
        range of the larger size, visible to every stage that declares one, so both stages must agree on the
        layout of the bytes they share (declare the same block in both, or let one stage use a prefix of it).
    */
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