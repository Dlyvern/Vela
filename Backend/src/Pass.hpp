#ifndef VELA_BACKEND_PASS_HPP
#define VELA_BACKEND_PASS_HPP

#include "volk.h"
#include "vk_mem_alloc.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace vela::backend
{
    struct AttachmentDescription
    {
        std::string name;
        VkFormat format;
        VkImageUsageFlags usage;
        float scale{1.0f};
    };

    struct Attachment
    {
        VkImage image{VK_NULL_HANDLE};
        VkImageView view{VK_NULL_HANDLE};
        VmaAllocation allocation{VK_NULL_HANDLE};
        VkFormat format{VK_FORMAT_UNDEFINED};
        VkExtent2D extent{};
    };

    struct PassContext
    {
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        VkImage colorImage{VK_NULL_HANDLE};
        VkImageView colorImageView{VK_NULL_HANDLE};
        VkExtent2D extent{};
        const std::unordered_map<std::string, Attachment>& attachments;
    };

    class Pass
    {
    public:
        virtual const std::vector<VkFormat>& getColorFormats() const = 0;
        virtual VkFormat getDepthFormat() const = 0;

        virtual void begin(const PassContext& passContext) = 0;
        virtual void end(const PassContext& passContext) = 0;

        virtual std::vector<AttachmentDescription> outputs() const = 0;
        
        virtual ~Pass() = default;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PASS_HPP
