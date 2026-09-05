#include "Vela/Graphics/Texture.hpp"

#include <stdexcept>
#include "TextureImpl.hpp"

namespace vela::graphics
{
    Result<Texture> Texture::create(core::Context& ctx, const ImageData& image)
    {
        try
        {
            return Texture(ctx, image);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::ImageLoadFailed, error.what()};
        }
    }

    Texture::Texture(core::Context& ctx, const ImageData& image) :
    m_impl(std::make_unique<backend::TextureImpl>(ctx, image))
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