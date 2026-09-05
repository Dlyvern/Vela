#include "Vela/Graphics/Material.hpp"

#include <stdexcept>
#include "MaterialImpl.hpp"
#include "TextureImpl.hpp"
#include "Vela/Graphics/Texture.hpp"

namespace vela::graphics
{
    Result<Material> Material::create(core::Context& ctx, const MaterialDescription& description)
    {
        try
        {
            return Material(ctx, description);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::PipelineCreationFailed, error.what()};
        }
    }

    Material::Material(core::Context& ctx, const MaterialDescription& description) :
    m_impl(std::make_unique<backend::MaterialImpl>(ctx, description))
    {

    }

    const MaterialDescription& Material::getMaterialDescription() const
    {
        return m_impl->getMaterialDescription();
    }

    Status Material::setTexture(uint32_t slot, const Texture& texture)
    {
        return m_impl->setTexture(slot, texture.impl()->getImageView(), texture.impl()->getSampler());
    }

    backend::MaterialImpl* Material::impl() const
    {
        return m_impl.get();
    }

    Material::~Material() = default;
    Material::Material(Material&&) noexcept = default;
    Material& Material::operator=(Material&&) noexcept = default;
} //namespace vela::graphics