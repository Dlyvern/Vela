#ifndef VELA_BACKEND_PRESENT_PASS_HPP
#define VELA_BACKEND_PRESENT_PASS_HPP

#include "Pass.hpp"

namespace vela::backend
{
    class PresentPass : public Pass
    {
    public:
        PresentPass(VkFormat colorFormat);

        void setClearColor(float r, float g, float b, float a);
        void setColorFormat(VkFormat colorFormat);

        const std::vector<VkFormat>& getColorFormats() const override;
        VkFormat getDepthFormat() const override;

        void begin(const PassContext& passContext) override;
        void end(const PassContext& passContext) override;
        std::vector<AttachmentDescription> outputs() const override;

    private:
        std::vector<VkFormat> m_colorFormats;
        VkFormat m_depthFormat{VK_FORMAT_D32_SFLOAT};
        VkClearValue m_clearValue{{{0.1f, 0.2f, 0.4f, 1.0f}}};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PRESENT_PASS_HPP
