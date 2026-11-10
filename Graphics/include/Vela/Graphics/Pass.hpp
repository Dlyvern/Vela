#ifndef VELA_GRAPHICS_PASS_HPP
#define VELA_GRAPHICS_PASS_HPP

#include "RenderTypes.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "DynamicBuffer.hpp"
#include "ComputeProgram.hpp"
#include "Sampler.hpp"

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

        /*
            Samples per pixel: 1 (the default) or a power of two up to core::DeviceInfo::maxMsaaSamples.
            Every color and depth output of one pass must use the same count, and addPass() rejects a
            count the device can't render. A multisampled attachment can't be a pass input or the present
            source; resolve it and read the resolve target instead
        */
        uint32_t samples{1};

        /*
            Only for samples > 1: name of a single-sampled attachment Vela resolves this one into when the
            pass ends (color: average of the samples, depth: sample zero). The resolve target has the same
            format and size, counts as an output of this pass for ordering, and is what later passes list
            as an input. Empty: no resolve; later passes can still load the multisampled attachment
        */
        std::string resolveTarget;

        /*
            Keeps the attachment's contents across frames instead of starting each frame undefined, e.g. for
            accumulation with LoadOp::Load. Vela clears it to `clear` whenever it is (re)created: on first use,
            after a resize (unless it has a fixed size) and after the graph changes. Single-sampled only. If any
            pass reads it as InputUsage::History it is double-buffered, and a pass that loads it then sees the
            frame before last, so write every pixel instead. Any pass declaring it persistent makes it persistent
        */
        bool persistent{false};
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

        /*
            Same as bindAttachment(slot, attachmentName), with the sampler the shader reads it through instead of
            the default (clamp to edge; linear, or nearest for depth). Pass a SamplerDescription with a compareOp
            to sample a depth attachment with hardware comparison (sampler2DShadow in GLSL), e.g. for PCF shadows.
            Linear filtering of a depth format with comparison is supported by all desktop GPUs but optional in Vulkan.
            slot:           pass-input slot (set 2 binding), below 4.
            attachmentName: an attachment the pass lists in its inputs.
            sampler:        how the attachment is sampled.
        */
        void bindAttachment(uint32_t slot, const std::string& attachmentName, const SamplerDescription& sampler);

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

        /*
            Draws `instanceCount` copies of a mesh in one call. The vertex shader tells them apart by
            gl_InstanceIndex, which runs from firstInstance to firstInstance + instanceCount - 1, typically to
            index per-instance data (say, model matrices) in a storage buffer of the material
            (Material::setStorageBuffer). Push constants are not written: if the material has any, call
            bind(material) and setConstants() first. Draws nothing when instanceCount is 0.
            mesh:          the mesh to draw.
            material:      the material to draw it with.
            instanceCount: copies to draw.
            firstInstance: gl_InstanceIndex of the first copy.
        */
        void drawInstanced(const Mesh& mesh, const Material& material, uint32_t instanceCount,
                 uint32_t firstInstance = 0);

        void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

        /*
            Clears a rectangle of the pass's depth output to `depth`, leaving the rest of it as it is, e.g. to
            re-render a few tiles of a persistent shadow atlas loaded with LoadOp::Load. Ignores the scissor.
            Does nothing if the pass has no depth output; the rectangle is clamped to the attachment.
            x, y:          top-left corner in texels.
            width, height: size in texels.
            depth:         the depth to write, 0 to 1.
        */
        void clearDepth(int32_t x, int32_t y, uint32_t width, uint32_t height, float depth = 1.0f);

        /*
            attachmentName: a persistent attachment (AttachmentSlot::persistent) of the graph.
            Returns:        true if Vela (re)created and cleared it at the start of this frame (first use, a
                            resize, a graph change), so whatever was cached in it before is gone. False for
                            attachments that kept their contents, and for non-persistent or unknown ones.
        */
        [[nodiscard]] bool wasAttachmentReset(const std::string& attachmentName) const;

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