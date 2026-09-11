#ifndef VELA_GRAPHICS_PASS_HPP
#define VELA_GRAPHICS_PASS_HPP

#include "RenderTypes.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "DynamicBuffer.hpp"

#include "Vela/Math/Matrix.hpp"

#include <string>
#include <vector>
#include <optional>

namespace vela::backend
{
    class RenderGraphImpl;
    class PassAdapter;
}

namespace vela::graphics
{
    struct AttachmentSlot
    {
        std::string name;
        TextureFormat format{TextureFormat::RGBA8Srgb};
        LoadOp load{LoadOp::Clear};
        StoreOp store{StoreOp::Store};
        ClearValue clear{};
        float scale{1.0f};
    };

    struct PassInput
    {
        std::string name;
        InputUsage usage{InputUsage::Sampled};
    };

    struct PassDescription
    {
        std::vector<AttachmentSlot> colorOutputs;
        std::optional<AttachmentSlot> depthOutput;
        std::vector<PassInput> inputs;
    };

    class PassRecorder
    {
    public:
        void draw(const Mesh& mesh, const Material& material, const math::Mat4& model);
        void bind(const Material& material);
        void bindVertexBuffer(const DynamicBuffer& buffer);
        void bindIndexBuffer(const DynamicBuffer& buffer);
        void setConstants(const math::Mat4& value);
        void drawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset,
                 uint32_t instanceCount = 1, uint32_t firstInstance = 0);
        void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

        [[nodiscard]] Extent2D extent() const;

    private:
        friend class backend::PassAdapter;

        backend::RenderGraphImpl* m_graph{nullptr};
        Extent2D m_extent{};
    };

    class Pass
    {
    public:
        virtual PassDescription describe() const = 0;
        virtual void record(PassRecorder& recorder) = 0;
        virtual ~Pass() = default;
    };

} //namespace vela::graphics

#endif //VELA_GRAPHICS_PASS_HPP