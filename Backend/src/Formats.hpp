#ifndef VELA_BACKEND_FORMATS_HPP
#define VELA_BACKEND_FORMATS_HPP

#include "volk.h"

#include "Pass.hpp"

#include "Vela/Graphics/RenderTypes.hpp"

namespace vela::backend
{
    [[nodiscard]] inline VkCullModeFlags toVkCullMode(graphics::CullMode cullMode)
    {   
        switch(cullMode)
        {
            case graphics::CullMode::None: return VK_CULL_MODE_NONE;
            case graphics::CullMode::Back: return VK_CULL_MODE_BACK_BIT;
            case graphics::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        }

        return VK_CULL_MODE_NONE;
    }

    [[nodiscard]] inline VkFrontFace toVkFrontFace(graphics::FrontFace frontFace)
    {       
        switch (frontFace)
        {
            case graphics::FrontFace::Clockwise:  return VK_FRONT_FACE_CLOCKWISE;
            case graphics::FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }

        return VK_FRONT_FACE_CLOCKWISE;
    }

    [[nodiscard]] inline VkCompareOp toVkCompare(graphics::DepthCompare depthCompare)
    {
        switch(depthCompare)
        {
            case graphics::DepthCompare::Never: return VK_COMPARE_OP_NEVER;
            case graphics::DepthCompare::Less:  return VK_COMPARE_OP_LESS;
            case graphics::DepthCompare::Equal: return VK_COMPARE_OP_EQUAL;
            case graphics::DepthCompare::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case graphics::DepthCompare::Greater: return VK_COMPARE_OP_GREATER;
            case graphics::DepthCompare::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case graphics::DepthCompare::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case graphics::DepthCompare::Always: return VK_COMPARE_OP_ALWAYS;
        }

        return VK_COMPARE_OP_ALWAYS;
    }

    [[nodiscard]] inline VkFormat toVkFormat(graphics::TextureFormat format)
    {
        switch (format)
        {
            case graphics::TextureFormat::RGBA8Srgb:    return VK_FORMAT_R8G8B8A8_SRGB;
            case graphics::TextureFormat::RGBA8Unorm:   return VK_FORMAT_R8G8B8A8_UNORM;
            case graphics::TextureFormat::RGBA16Float:  return VK_FORMAT_R16G16B16A16_SFLOAT;
            case graphics::TextureFormat::Depth32Float: return VK_FORMAT_D32_SFLOAT;
        }

        return VK_FORMAT_UNDEFINED;
    }

    [[nodiscard]] inline VkAttachmentLoadOp toVkLoadOp(graphics::LoadOp load)
    {
        switch (load)
        {
            case graphics::LoadOp::Clear:    return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case graphics::LoadOp::Load:     return VK_ATTACHMENT_LOAD_OP_LOAD;
            case graphics::LoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }

        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    [[nodiscard]] inline VkAttachmentStoreOp toVkStoreOp(graphics::StoreOp store)
    {
        switch (store)
        {
            case graphics::StoreOp::Store:    return VK_ATTACHMENT_STORE_OP_STORE;
            case graphics::StoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }

    [[nodiscard]] inline InputUsage toBackendInputUsage(graphics::InputUsage usage)
    {
        return usage == graphics::InputUsage::TransferSource
            ? InputUsage::TransferSource
            : InputUsage::Sampled;
    }

    [[nodiscard]] inline VkClearValue toVkClearValue(const graphics::ClearValue& clear, bool isDepth)
    {
        VkClearValue value{};

        if (isDepth)
            value.depthStencil = {clear.depth, clear.stencil};
        else
            value.color = {{clear.r, clear.g, clear.b, clear.a}};

        return value;
    }
} //namespace vela::backend

#endif //VELA_BACKEND_FORMATS_HPP
