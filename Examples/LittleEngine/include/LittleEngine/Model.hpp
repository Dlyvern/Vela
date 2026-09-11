#ifndef LITTLE_ENGINE_MODEL_HPP
#define LITTLE_ENGINE_MODEL_HPP

#include "Vela/Core/Context.hpp"
#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"
#include "Vela/Graphics/Texture.hpp"
#include "Vela/Math/Matrix.hpp"
#include "Vela/Math/Vector.hpp"
#include "Vela/Result.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace little
{
    struct Bounds
    {
        vela::math::Vector3f min{0.0f, 0.0f, 0.0f};
        vela::math::Vector3f max{0.0f, 0.0f, 0.0f};

        bool empty{true};

        void expand(const vela::math::Vector3f& point);

        [[nodiscard]] vela::math::Vector3f center() const;
        [[nodiscard]] float diagonal() const;
    };

    struct Instance
    {
        size_t meshIndex{0};
        size_t materialIndex{0};
        vela::math::Mat4 transform{1.0f};
    };

    struct Model
    {
        std::vector<vela::graphics::Mesh> meshes;
        std::vector<vela::graphics::Material> materials;
        std::vector<vela::graphics::Texture> textures;
        std::vector<Instance> instances;

        Bounds bounds;

        double parseMilliseconds{0.0};
        double imageMilliseconds{0.0};
        double uploadMilliseconds{0.0};
    };

    [[nodiscard]] vela::Result<Model> loadModel(vela::core::Context& context, const std::string& path,
        std::span<const uint32_t> vertexShader, std::span<const uint32_t> fragmentShader);
} //namespace little

#endif //LITTLE_ENGINE_MODEL_HPP
