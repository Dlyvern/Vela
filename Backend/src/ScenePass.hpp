#ifndef VELA_BACKEND_SCENE_PASS_HPP
#define VELA_BACKEND_SCENE_PASS_HPP 

#include "Pass.hpp"

//TODO place swapchain images to Attachments 

namespace vela::backend
{   
    class ScenePass : public Pass
    {
    public:
        ScenePass();
        PassDeclaration declare() const override;
        void record(const PassContext& passContext) override;
    private:
        VkFormat m_depthFormat{VK_FORMAT_D32_SFLOAT};
        VkClearValue m_clearValue{{{0.1f, 0.2f, 0.4f, 1.0f}}};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_SCENE_PASS_HPP