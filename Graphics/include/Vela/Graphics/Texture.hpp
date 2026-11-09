#ifndef VELA_GRAPHICS_TEXTURE_HPP
#define VELA_GRAPHICS_TEXTURE_HPP

#include <memory>

#include "ImageData.hpp"
#include "Sampler.hpp"

#include "Vela/Result.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class TextureImpl;
} //namespace vela::backend

namespace vela::graphics
{
    // How a texture is created, beyond its pixels and sampler
    struct TextureOptions
    {
        /*
            Builds the full mip chain on the GPU from the image's single level (a chain of linear blits, which
            for sRGB formats filters in linear space). The image must have levels == 1 and an uncompressed
            format the device can filter-blit (true for RGBA8, RGBA16Float and the other color formats on
            desktop GPUs); block-compressed formats are rejected. Texture::update() regenerates the mips
        */
        bool generateMips{false};
    };

    // A sampled 2D image. Uploads are queued on the GPU and never wait for the device to go idle:
    // create() and update() return once the pixels are copied into staging memory, and every frame
    // submitted afterwards sees the new pixels
    class Texture
    {
    public:
        // Creates a texture from image, which holds every mip level it provides, largest first.
        // ctx:                Vela context that owns the GPU memory.
        // image:              the pixels; only read during the call.
        // samplerDescription: how shaders sample it. Anisotropy only applies with Feature::SamplerAnisotropy
        //                     enabled and is clamped to DeviceInfo::maxAnisotropy.
        // Returns:            the texture, or an error for an empty or too-small image, a depth format, a BC
        //                     format without Feature::TextureCompressionBC, or a failed allocation.
        static Result<Texture> create(core::Context& ctx, const ImageData& image, const SamplerDescription& samplerDescription = {});

        // Same as above, with options such as GPU mip generation.
        // options: see TextureOptions; an unsupported combination makes this fail.
        static Result<Texture> create(core::Context& ctx, const ImageData& image, const SamplerDescription& samplerDescription,
            const TextureOptions& options);

        ~Texture();
        
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        /*
            Replaces every pixel of the texture, e.g. a font atlas that gained glyphs or a CPU-drawn image.
            Does not wait for the GPU. Frames already submitted sample the old pixels, everything recorded
            afterwards (including the frame being recorded, if called from a pass) the new ones.
            Meant for occasional updates, not for streaming every frame.
            image:   the new pixels; width, height, format and levels must match the image the texture was created
                     from (with generated mips: one level, and the mips are regenerated).
            Returns: an error if they don't match, the pixel buffer is too small, or the upload could not be allocated.
        */
        [[nodiscard]] Status update(const ImageData& image);

        backend::TextureImpl* impl() const;
    private:
        Texture(core::Context& ctx, const ImageData& image, const SamplerDescription& samplerDescription,
            const TextureOptions& options);

        std::unique_ptr<backend::TextureImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_TEXTURE_HPP