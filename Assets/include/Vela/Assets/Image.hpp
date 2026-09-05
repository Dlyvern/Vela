#ifndef VELA_ASSETS_IMAGE_HPP
#define VELA_ASSETS_IMAGE_HPP

#include "Vela/Graphics/ImageData.hpp"
#include "Vela/Result.hpp"

#include <string>

namespace vela::assets
{
    class Image
    {
    public:
        static Result<Image> load(const std::string& path,
            graphics::TextureFormat format = graphics::TextureFormat::RGBA8Srgb);

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        Image(Image&&) noexcept;
        Image& operator=(Image&&) noexcept;

        ~Image();

        [[nodiscard]] graphics::ImageData data() const;

        [[nodiscard]] uint32_t width() const;
        [[nodiscard]] uint32_t height() const;

    private:
        Image() = default;

        unsigned char* m_pixels{nullptr};
        uint32_t m_width{0};
        uint32_t m_height{0};
        graphics::TextureFormat m_format{graphics::TextureFormat::RGBA8Srgb};
    };
} //namespace vela::assets

#endif //VELA_ASSETS_IMAGE_HPP
