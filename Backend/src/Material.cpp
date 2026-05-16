#include "Vela/Graphics/Material.hpp"
#include "MaterialImpl.hpp"
#include "TextureImpl.hpp"
#include "Vela/Graphics/Texture.hpp"

namespace vela::graphics
{
    Material::Material(core::Context& ctx, const std::string& vertShaderPath, const std::string& fragShaderPath) :
    m_impl(std::make_unique<backend::MaterialImpl>(ctx, vertShaderPath, fragShaderPath))
    {

    }

    void Material::setAlbedoTexture(const Texture& texture)
    {
        m_impl->setAlbedoTexture(texture.impl()->getImageView(), texture.impl()->getSampler());
    }

    void Material::setMVP(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
    {
        m_impl->setMVP(model, view, projection);;
    }

    backend::MaterialImpl* Material::impl() const
    {
        return m_impl.get();
    }

    Material::~Material() = default;
    Material::Material(Material&&) noexcept = default;
    Material& Material::operator=(Material&&) noexcept = default;
} //namespace vela::graphics