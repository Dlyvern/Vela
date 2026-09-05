#include "Vela/Assets/Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace vela::assets
{
    Result<Image> Image::load(const std::string& path, graphics::TextureFormat format)
    {
        int width{0};
        int height{0};
        int channels{0};

        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (pixels == nullptr)
        {
            const char* reason = stbi_failure_reason();
            return Error{ErrorCode::ImageLoadFailed,
                "Failed to load image " + path + ": " + (reason != nullptr ? reason : "unknown")};
        }

        Image image;
        image.m_pixels = pixels;
        image.m_width = static_cast<uint32_t>(width);
        image.m_height = static_cast<uint32_t>(height);
        image.m_format = format;

        return std::move(image);
    }

    Image::Image(Image&& other) noexcept :
    m_pixels(other.m_pixels), m_width(other.m_width), m_height(other.m_height), m_format(other.m_format)
    {
        other.m_pixels = nullptr;
    }

    Image& Image::operator=(Image&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pixels != nullptr)
                stbi_image_free(m_pixels);

            m_pixels = other.m_pixels;
            m_width = other.m_width;
            m_height = other.m_height;
            m_format = other.m_format;

            other.m_pixels = nullptr;
        }

        return *this;
    }

    Image::~Image()
    {
        if (m_pixels != nullptr)
            stbi_image_free(m_pixels);
    }

    graphics::ImageData Image::data() const
    {
        const size_t size = static_cast<size_t>(m_width) * m_height * graphics::bytesPerPixel(m_format);

        return graphics::ImageData
        {
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(m_pixels), size),
            m_width,
            m_height,
            m_format
        };
    }

    uint32_t Image::width() const
    {
        return m_width;
    }

    uint32_t Image::height() const
    {
        return m_height;
    }
} //namespace vela::assets
