#include "LittleEngine/Model.hpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "Vela/Assets/Image.hpp"
#include "Vela/Graphics/Vertex.hpp"

#include <ktx.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <thread>
#include <unordered_map>
#include <utility>

namespace little
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        double millisecondsSince(Clock::time_point start)
        {
            return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        }

        vela::math::Mat4 toMat4(const cgltf_float source[16])
        {
            vela::math::Mat4 result{};
            std::memcpy(result.data(), source, sizeof(float) * 16);

            return result;
        }

        vela::math::Vector3f transformPoint(const vela::math::Mat4& matrix, const vela::math::Vector3f& point)
        {
            const float* m = matrix.data();

            return
            {
                m[0] * point.x + m[4] * point.y + m[8]  * point.z + m[12],
                m[1] * point.x + m[5] * point.y + m[9]  * point.z + m[13],
                m[2] * point.x + m[6] * point.y + m[10] * point.z + m[14]
            };
        }

        struct KtxImage
        {
            KtxImage() = default;

            ~KtxImage()
            {
                if (texture != nullptr)
                    ktxTexture_Destroy(ktxTexture(texture));
            }

            KtxImage(KtxImage&& other) noexcept : texture(other.texture)
            {
                other.texture = nullptr;
            }

            KtxImage& operator=(KtxImage&& other) noexcept
            {
                std::swap(texture, other.texture);

                return *this;
            }

            KtxImage(const KtxImage&) = delete;
            KtxImage& operator=(const KtxImage&) = delete;

            ktxTexture2* texture{nullptr};
        };

        struct DecodedImage
        {
            [[nodiscard]] bool valid() const
            {
                return ktx.texture != nullptr || stb.has_value();
            }

            [[nodiscard]] vela::graphics::ImageData data() const
            {
                if (ktx.texture == nullptr)
                    return stb->data();

                vela::graphics::ImageData imageData{};
                imageData.pixels = std::span(
                    reinterpret_cast<const std::byte*>(ktx.texture->pData), ktx.texture->dataSize);
                imageData.width = ktx.texture->baseWidth;
                imageData.height = ktx.texture->baseHeight;
                imageData.format = vela::graphics::TextureFormat::BC7Srgb;
                imageData.levels = ktx.texture->numLevels;

                return imageData;
            }

            void reset()
            {
                stb.reset();
                ktx = KtxImage{};
            }

            std::optional<vela::assets::Image> stb;
            KtxImage ktx;
        };

        DecodedImage loadImage(const std::filesystem::path& path)
        {
            DecodedImage result{};

            if (path.extension() != ".ktx2")
            {
                if (auto loaded = vela::assets::Image::load(path.string()); loaded)
                    result.stb = std::move(loaded).value();

                return result;
            }

            ktxTexture2* texture = nullptr;

            if (ktxTexture2_CreateFromNamedFile(path.string().c_str(),
                    KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture) != KTX_SUCCESS)
                return result;

            if (ktxTexture2_NeedsTranscoding(texture)
                && ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC7_RGBA, 0) != KTX_SUCCESS)
            {
                ktxTexture_Destroy(ktxTexture(texture));

                return result;
            }

            result.ktx.texture = texture;

            return result;
        }

        vela::Result<vela::graphics::Texture> whiteTexture(vela::core::Context& context)
        {
            static constexpr std::array<uint8_t, 4> pixels{255, 255, 255, 255};

            vela::graphics::ImageData imageData{};
            imageData.width = 1;
            imageData.height = 1;
            imageData.format = vela::graphics::TextureFormat::RGBA8Srgb;
            imageData.pixels = std::as_bytes(std::span<const uint8_t>(pixels));

            return vela::graphics::Texture::create(context, imageData);
        }
    } //namespace

    void Bounds::expand(const vela::math::Vector3f& point)
    {
        if (empty)
        {
            min = point;
            max = point;
            empty = false;

            return;
        }

        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);

        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    vela::math::Vector3f Bounds::center() const
    {
        return {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f};
    }

    float Bounds::diagonal() const
    {
        const float x = max.x - min.x;
        const float y = max.y - min.y;
        const float z = max.z - min.z;

        return std::sqrt(x * x + y * y + z * z);
    }

    vela::Result<Model> loadModel(vela::core::Context& context, const std::string& path,
        std::span<const uint32_t> vertexShader, std::span<const uint32_t> fragmentShader)
    {
        cgltf_options options{};
        cgltf_data* data = nullptr;

        if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
            return vela::Error{vela::ErrorCode::InvalidArgument, "Failed to parse " + path};

        if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
        {
            cgltf_free(data);
            return vela::Error{vela::ErrorCode::InvalidArgument, "Failed to load buffers for " + path};
        }

        Model model;

        const std::filesystem::path baseDirectory = std::filesystem::path(path).parent_path();

        const auto parseStart = Clock::now();

        std::vector<std::pair<size_t, size_t>> primitiveRanges(data->meshes_count);
        std::vector<size_t> primitiveMaterials;
        std::vector<Bounds> primitiveBounds;

        for (size_t meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex)
        {
            const cgltf_mesh& mesh = data->meshes[meshIndex];

            primitiveRanges[meshIndex] = {model.meshes.size(), 0};

            for (size_t primitiveIndex = 0; primitiveIndex < mesh.primitives_count; ++primitiveIndex)
            {
                const cgltf_primitive& primitive = mesh.primitives[primitiveIndex];

                const cgltf_accessor* positionAccessor = nullptr;
                const cgltf_accessor* uvAccessor = nullptr;
                const cgltf_accessor* normalAccessor = nullptr;

                for (cgltf_size index = 0; index < primitive.attributes_count; ++index)
                {
                    const cgltf_attribute& attribute = primitive.attributes[index];

                    if (attribute.type == cgltf_attribute_type_position)
                        positionAccessor = attribute.data;
                    else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0)
                        uvAccessor = attribute.data;
                    else if (attribute.type == cgltf_attribute_type_normal)
                        normalAccessor = attribute.data;
                }

                if (positionAccessor == nullptr || primitive.indices == nullptr)
                    continue;

                std::vector<vela::graphics::StaticVertex> vertices(positionAccessor->count);
                Bounds localBounds;

                for (cgltf_size index = 0; index < positionAccessor->count; ++index)
                {
                    vela::graphics::StaticVertex& vertex = vertices[index];

                    cgltf_accessor_read_float(positionAccessor, index, vertex.position, 3);

                    if (uvAccessor != nullptr)
                        cgltf_accessor_read_float(uvAccessor, index, vertex.uv, 2);

                    if (normalAccessor != nullptr)
                        cgltf_accessor_read_float(normalAccessor, index, vertex.normal, 3);

                    localBounds.expand({vertex.position[0], vertex.position[1], vertex.position[2]});
                }

                std::vector<uint32_t> indices(primitive.indices->count);

                for (cgltf_size index = 0; index < primitive.indices->count; ++index)
                    indices[index] = static_cast<uint32_t>(cgltf_accessor_read_index(primitive.indices, index));

                auto mesh = vela::graphics::Mesh::create(context, vertices, indices);

                if (!mesh)
                {
                    std::cerr << "Failed to create mesh: " << mesh.error().message << '\n';
                    continue;
                }

                model.meshes.push_back(std::move(mesh).value());
                primitiveBounds.push_back(localBounds);

                primitiveMaterials.push_back(primitive.material != nullptr
                    ? static_cast<size_t>(primitive.material - data->materials)
                    : data->materials_count);

                ++primitiveRanges[meshIndex].second;
            }
        }

        model.parseMilliseconds = millisecondsSince(parseStart);

        auto baseColorImage = [](const cgltf_material& material) -> const cgltf_image*
        {
            if (!material.has_pbr_metallic_roughness)
                return nullptr;

            const cgltf_texture* texture = material.pbr_metallic_roughness.base_color_texture.texture;

            if (texture == nullptr || texture->image == nullptr || texture->image->uri == nullptr)
                return nullptr;

            return texture->image;
        };

        std::vector<const cgltf_image*> uniqueImages;
        std::unordered_map<const cgltf_image*, size_t> imageSlots;

        for (size_t materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
        {
            const cgltf_image* image = baseColorImage(data->materials[materialIndex]);

            if (image == nullptr || imageSlots.contains(image))
                continue;

            imageSlots[image] = uniqueImages.size();
            uniqueImages.push_back(image);
        }

        std::vector<DecodedImage> decoded(uniqueImages.size());

        const auto imageStart = Clock::now();

        if (!uniqueImages.empty())
        {
            const unsigned threadCount = std::min<unsigned>(
                std::max(std::thread::hardware_concurrency(), 1u),
                static_cast<unsigned>(uniqueImages.size()));

            std::atomic<size_t> nextImage{0};
            std::vector<std::thread> workers;
            workers.reserve(threadCount);

            for (unsigned worker = 0; worker < threadCount; ++worker)
            {
                workers.emplace_back([&]
                {
                    for (size_t index = nextImage++; index < uniqueImages.size(); index = nextImage++)
                        decoded[index] = loadImage(baseDirectory / uniqueImages[index]->uri);
                });
            }

            for (std::thread& worker : workers)
                worker.join();
        }

        model.imageMilliseconds = millisecondsSince(imageStart);

        model.textures.reserve(uniqueImages.size() + 1);

        const auto uploadStart = Clock::now();

        auto fallbackTextureResult = whiteTexture(context);

        if (!fallbackTextureResult)
        {
            cgltf_free(data);
            return vela::Error{vela::ErrorCode::AllocationFailed, "Failed to create fallback texture"};
        }

        const size_t fallbackTexture = model.textures.size();
        model.textures.emplace_back(std::move(fallbackTextureResult).value());

        std::vector<size_t> imageTextures(uniqueImages.size(), fallbackTexture);

        for (size_t index = 0; index < uniqueImages.size(); ++index)
        {
            if (!decoded[index].valid())
            {
                std::cerr << "Failed to decode " << uniqueImages[index]->uri << '\n';
                continue;
            }

            auto texture = vela::graphics::Texture::create(context, decoded[index].data());

            if (!texture)
            {
                std::cerr << "Failed to upload " << uniqueImages[index]->uri << '\n';
                continue;
            }

            imageTextures[index] = model.textures.size();
            model.textures.emplace_back(std::move(texture).value());

            decoded[index].reset();
        }

        model.uploadMilliseconds = millisecondsSince(uploadStart);

        decoded.clear();

        model.materials.reserve(data->materials_count + 1);

        auto createMaterial = [&](size_t textureIndex) -> bool
        {
            auto material = vela::graphics::Material::create(context,
            {
                .vertexShader = vertexShader,
                .fragmentShader = fragmentShader,
                .vertexLayout = vela::graphics::StaticVertex::layout()
            });

            if (!material)
            {
                std::cerr << "Failed to create material: " << material.error().message << '\n';
                return false;
            }

            vela::graphics::Material& stored = model.materials.emplace_back(std::move(material).value());

            return static_cast<bool>(stored.setTexture(0, model.textures[textureIndex]));
        };

        for (size_t materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
        {
            const cgltf_image* image = baseColorImage(data->materials[materialIndex]);

            const size_t textureIndex = image != nullptr
                ? imageTextures[imageSlots[image]]
                : fallbackTexture;

            if (!createMaterial(textureIndex))
            {
                cgltf_free(data);
                return vela::Error{vela::ErrorCode::AllocationFailed, "Failed to create material"};
            }
        }

        if (!createMaterial(fallbackTexture))
        {
            cgltf_free(data);
            return vela::Error{vela::ErrorCode::AllocationFailed, "Failed to create fallback material"};
        }

        for (size_t nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
        {
            const cgltf_node& node = data->nodes[nodeIndex];

            if (node.mesh == nullptr)
                continue;

            cgltf_float worldMatrix[16];
            cgltf_node_transform_world(&node, worldMatrix);

            const vela::math::Mat4 transform = toMat4(worldMatrix);

            const size_t meshIndex = static_cast<size_t>(node.mesh - data->meshes);
            const auto [first, count] = primitiveRanges[meshIndex];

            for (size_t offset = 0; offset < count; ++offset)
            {
                const size_t primitive = first + offset;

                model.instances.push_back({primitive, primitiveMaterials[primitive], transform});

                const Bounds& local = primitiveBounds[primitive];

                for (int corner = 0; corner < 8; ++corner)
                {
                    const vela::math::Vector3f point
                    {
                        (corner & 1) ? local.max.x : local.min.x,
                        (corner & 2) ? local.max.y : local.min.y,
                        (corner & 4) ? local.max.z : local.min.z
                    };

                    model.bounds.expand(transformPoint(transform, point));
                }
            }
        }

        cgltf_free(data);

        return model;
    }
} //namespace little
