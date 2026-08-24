#ifndef VELA_BACKEND_PASS_HPP
#define VELA_BACKEND_PASS_HPP

#include "volk.h"

#include <vector>

namespace vela::backend
{
    struct PassContext
    {
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        VkImage colorImage{VK_NULL_HANDLE};
        VkImageView colorImageView{VK_NULL_HANDLE};
        VkImage depthImage{VK_NULL_HANDLE};
        VkImageView depthImageView{VK_NULL_HANDLE};
        VkExtent2D extent{};
    };

    class Pass
    {
    public:
        virtual const std::vector<VkFormat>& getColorFormats() const = 0;
        virtual VkFormat getDepthFormat() const = 0;

        virtual void begin(const PassContext& passContext) = 0;
        virtual void end(const PassContext& passContext) = 0;

        virtual ~Pass() = default;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PASS_HPP
