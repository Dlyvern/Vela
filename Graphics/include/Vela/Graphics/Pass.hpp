#ifndef VELA_GRAPHICS_PASS_HPP
#define VELA_GRAPHICS_PASS_HPP

#include "RenderTypes.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "DynamicBuffer.hpp"
#include "ComputeProgram.hpp"

#include "Vela/Math/Matrix.hpp"

#include <string>
#include <vector>
#include <optional>
#include <type_traits>

namespace vela::backend
{
    class RenderGraphImpl;
    class PassAdapter;
    class ComputePassAdapter;
}

namespace vela::graphics
{
    class Texture;

    struct AttachmentSlot
    {
        std::string name;
        TextureFormat format{TextureFormat::RGBA8Srgb};
        LoadOp load{LoadOp::Clear};
        StoreOp store{StoreOp::Store};
        ClearValue clear{};
        float scale{1.0f};

        //Use if you want fixed-size attachment. Note that if attachment has size > 0, it won't be destroyed on swapchain resize
        Extent2D size{};
    };

    struct BufferSlot
    {
        std::string name;
        size_t size{0};
    };

    enum class BufferAccess : uint8_t
    {
        Read = 0,
        ReadWrite
    };

    struct BufferInput
    {
        std::string name;
        BufferAccess access{BufferAccess::Read};
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
        std::vector<BufferSlot> bufferOutputs;
        std::vector<BufferInput> bufferInputs;
    };

    class PassRecorder
    {
    public:
        void draw(const Mesh& mesh, const Material& material, const math::Mat4& model);

        // Same as draw(), but pushes a whole constants block instead of only a model matrix
        void drawWithConstants(const Mesh& mesh, const Material& material, const void* constants, uint32_t size);

        template<typename T> requires (std::is_trivially_copyable_v<T> && !std::is_same_v<T, math::Mat4>)
        void draw(const Mesh& mesh, const Material& material, const T& constants)
        {
            drawWithConstants(mesh, material, &constants, static_cast<uint32_t>(sizeof(T)));
        }
        void bind(const Material& material);
        void bindVertexBuffer(const DynamicBuffer& buffer);
        void bindIndexBuffer(const DynamicBuffer& buffer);
        void bindStorageBuffer(uint32_t slot, const std::string& bufferName);

        /*
            Pass-input slots (set 2). Each draw sees the slots as they were when it was recorded,
            so binding a different texture between draws is fine - e.g. one texture per ImGui command.
            bindAttachment only works for attachments the pass lists in its inputs.
        */
        void bindAttachment(uint32_t slot, const std::string& attachmentName);
        void bindTexture(uint32_t slot, const Texture& texture);

        void setConstants(const math::Mat4& value);
        void setConstantBytes(const void* data, uint32_t size);

        template<typename T> requires std::is_trivially_copyable_v<T>
        void setConstants(const T& value)
        {
            setConstantBytes(&value, static_cast<uint32_t>(sizeof(T)));
        }

        void drawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset,
                 uint32_t instanceCount = 1, uint32_t firstInstance = 0);
        void drawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                 uint32_t firstVertex = 0, uint32_t firstInstance = 0);
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

    class ComputeRecorder
    {
    public:
        void bind(const ComputeProgram& program);
        void bindStorageBuffer(uint32_t slot, const std::string& bufferName);
        void setConstants(const math::Mat4& value);
        void dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);

    private:
        friend class backend::ComputePassAdapter;

        backend::RenderGraphImpl* m_graph{nullptr};
        const ComputeProgram* m_program{nullptr};
    };

    class ComputePass
    {
    public:
        virtual PassDescription describe() const = 0;
        virtual void record(ComputeRecorder& recorder) = 0;
        virtual ~ComputePass() = default;
    };

} //namespace vela::graphics

#endif //VELA_GRAPHICS_PASS_HPP