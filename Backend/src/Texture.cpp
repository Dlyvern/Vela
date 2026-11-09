#include "Vela/Graphics/Texture.hpp"

#include <stdexcept>
#include "TextureImpl.hpp"

namespace vela::graphics
{
    Result<Texture> Texture::create(core::Context& ctx, const ImageData& image, const SamplerDescription& samplerDescription)
    {
        return create(ctx, image, samplerDescription, TextureOptions{});
    }

    Result<Texture> Texture::create(core::Context& ctx, const ImageData& image, const SamplerDescription& samplerDescription,
        const TextureOptions& options)
    {
        try
        {
            return Texture(ctx, image, samplerDescription, options);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::ImageLoadFailed, error.what()};
        }
    }

    Texture::Texture(core::Context& ctx, const ImageData& image, const SamplerDescription& samplerDescription,
        const TextureOptions& options) :
    m_impl(std::make_unique<backend::TextureImpl>(ctx, image, samplerDescription, options))
    {
         
    }

    Status Texture::update(const ImageData& image)
    {
        return m_impl->update(image);
    }

    backend::TextureImpl* Texture::impl() const
    {
        return m_impl.get();
    }

    Texture::~Texture() = default;
    Texture::Texture(Texture&&) noexcept = default;
    Texture& Texture::operator=(Texture&&) noexcept = default;

} //namespace vela::graphics