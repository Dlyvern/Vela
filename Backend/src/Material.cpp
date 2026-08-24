#include "Vela/Graphics/Material.hpp"
#include "MaterialImpl.hpp"
#include "TextureImpl.hpp"
#include "Vela/Graphics/Texture.hpp"
#include "Vela/Graphics/RenderGraph.hpp"
#include "RenderGraphImpl.hpp"

namespace vela::graphics
{
    Material::Material(core::Context& ctx, RenderGraph& renderGraph, const MaterialDescription& description) :
    m_impl(std::make_unique<backend::MaterialImpl>(ctx, *renderGraph.impl(), description))
    {

    }

    void Material::setAlbedoTexture(const Texture& texture)
    {
        m_impl->setAlbedoTexture(texture.impl()->getImageView(), texture.impl()->getSampler());
    }

    void Material::setMVP(const glm::mat4& view, const glm::mat4& projection)
    {
        m_impl->setMVP(view, projection);;
    }

    backend::MaterialImpl* Material::impl() const
    {
        return m_impl.get();
    }

    Material::~Material() = default;
    Material::Material(Material&&) noexcept = default;
    Material& Material::operator=(Material&&) noexcept = default;
} //namespace vela::graphics