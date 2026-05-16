#include "Vela/Graphics/Texture.hpp"
#include "TextureImpl.hpp"

namespace vela::graphics
{
    Texture::Texture(core::Context& ctx, const std::string& path) :
    m_impl(std::make_unique<backend::TextureImpl>(ctx, path))
    {
         
    }

    backend::TextureImpl* Texture::impl() const
    {
        return m_impl.get();
    }

    Texture::~Texture() = default;
    Texture::Texture(Texture&&) noexcept = default;
    Texture& Texture::operator=(Texture&&) noexcept = default;

} //namespace vela::graphics