#include "PresentPass.hpp"

#include <array>
#include <iostream>

namespace vela::backend
{
    PassDeclaration PresentPass::declare() const
    {
        PassInput colorInput{};
        colorInput.name = "color";
        colorInput.usage = InputUsage::TransferSource;

        PassDeclaration declaration{};
        declaration.inputs.push_back(colorInput);

        return declaration;
    }

    void PresentPass::record(const PassContext& passContext)
    {
        auto colorIt = passContext.attachments.find("color");

        if (colorIt == passContext.attachments.end())
        {
            std::cerr << "PresentPass: missing input attachment \"color\"\n";
            return;
        }

        auto swapchainIt = passContext.attachments.find("swapchain");

        if(swapchainIt == passContext.attachments.end())
        {
            std::cerr << "PresentPass: missing input attachment \"swapchain\"\n";
            return;
        }

        const Attachment& swapchain = swapchainIt->second;
        const Attachment& color = colorIt->second;

        VkImageBlit2 region{VK_STRUCTURE_TYPE_IMAGE_BLIT_2};
        region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.srcOffsets[0] = {0, 0, 0};
        region.srcOffsets[1] = {static_cast<int32_t>(color.extent.width),
                                static_cast<int32_t>(color.extent.height), 1};
        region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.dstOffsets[0] = {0, 0, 0};
        region.dstOffsets[1] = {static_cast<int32_t>(passContext.extent.width),
                                static_cast<int32_t>(passContext.extent.height), 1};

        VkBlitImageInfo2 blit{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
        blit.srcImage = color.image;
        blit.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blit.dstImage = swapchain.image;
        blit.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blit.regionCount = 1;
        blit.pRegions = &region;
        blit.filter = VK_FILTER_LINEAR;

        vkCmdBlitImage2(passContext.commandBuffer, &blit);
    }
} //namespace vela::backend
