#ifndef VELA_GRAPHICS_MATERIAL_HPP
#define VELA_GRAPHICS_MATERIAL_HPP

#include <memory>

#include "glm/mat4x4.hpp"

namespace vela::backend
{
    class MaterialImpl;
} //namespace vela::backend

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::graphics
{
    class Texture;
} //namespace vela::graphics

namespace vela::graphics
{
    class Material
    {
    public:
        Material(core::Context& ctx, const std::string& vertShaderPath, const std::string& fragShaderPath);
        ~Material();

        Material(Material&&) noexcept;
        Material& operator=(Material&&) noexcept;

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;

        void setAlbedoTexture(const Texture& texture);
        void setMVP(const glm::mat4& view, const glm::mat4& projection);

        backend::MaterialImpl* impl() const;
    private:
        std::unique_ptr<backend::MaterialImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_MATERIAL_HPP