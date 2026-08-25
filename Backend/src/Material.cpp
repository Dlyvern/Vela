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

    const MaterialDescription& Material::getMaterialDescription() const
    {
        return m_impl->getMaterialDescription();
    }

    void Material::setAlbedoTexture(const Texture& texture)
    {
        m_impl->setAlbedoTexture(texture.impl()->getImageView(), texture.impl()->getSampler());
    }

    backend::MaterialImpl* Material::impl() const
    {
        return m_impl.get();
    }

    Material::~Material() = default;
    Material::Material(Material&&) noexcept = default;
    Material& Material::operator=(Material&&) noexcept = default;
} //namespace vela::graphics