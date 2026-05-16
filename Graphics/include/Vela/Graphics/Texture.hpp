#ifndef VELA_GRAPHICS_TEXTURE_HPP
#define VELA_GRAPHICS_TEXTURE_HPP

#include <memory>
#include <string>

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
    class Texture
    {
    public:
        Texture(core::Context& ctx, const std::string& path);
        ~Texture();
        
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        backend::TextureImpl* impl() const;
    private:
        std::unique_ptr<backend::TextureImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_TEXTURE_HPP