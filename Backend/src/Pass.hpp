#ifndef VELA_BACKEND_PASS_HPP
#define VELA_BACKEND_PASS_HPP

#include "volk.h"
#include "vk_mem_alloc.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace vela::backend
{
    struct Attachment
    {
        VkImage image{VK_NULL_HANDLE};
        VkImageView view{VK_NULL_HANDLE};
        VmaAllocation allocation{VK_NULL_HANDLE};
        VkFormat format{VK_FORMAT_UNDEFINED};
        VkExtent2D extent{};
        bool external{false};
    };

    struct PassContext
    {
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        VkExtent2D extent{};
        const std::unordered_map<std::string, Attachment>& attachments;

        PassContext(const std::unordered_map<std::string, Attachment>& otherAttachments) : attachments(otherAttachments)
        {
            
        }
    };

    struct AttachmentOutput
    {
        std::string name;
        VkFormat format;
        VkAttachmentLoadOp load{VK_ATTACHMENT_LOAD_OP_CLEAR};
        VkAttachmentStoreOp store{VK_ATTACHMENT_STORE_OP_STORE};
        VkClearValue clear{};
        float scale{1.0f};
    };

    enum class InputUsage : uint8_t { Sampled = 0, TransferSource };

    struct PassInput
    {
        std::string name;
        InputUsage usage{InputUsage::Sampled};
    };

    struct PassDeclaration
    {
        std::vector<AttachmentOutput> colorOutputs;
        std::optional<AttachmentOutput> depthOutput;
        std::vector<PassInput> inputs;
    };

    struct PassFormats
    {
        std::vector<VkFormat> colorFormats;
        VkFormat depthFormat{VK_FORMAT_UNDEFINED};
    };

    [[nodiscard]] inline PassFormats formatsOf(const PassDeclaration& declaration)
    {
        PassFormats formats;
        formats.colorFormats.reserve(declaration.colorOutputs.size());

        for (const AttachmentOutput& colorOutput : declaration.colorOutputs)
            formats.colorFormats.push_back(colorOutput.format);

        if (declaration.depthOutput.has_value())
            formats.depthFormat = declaration.depthOutput->format;

        return formats;
    }

    class Pass
    {
    public:
        virtual PassDeclaration declare() const = 0;
        virtual void record(const PassContext& passContext) = 0;
        virtual ~Pass() = default;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PASS_HPP
