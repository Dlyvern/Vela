#ifndef VELA_BACKEND_PASS_ADAPTER_HPP
#define VELA_BACKEND_PASS_ADAPTER_HPP

#include "Pass.hpp"

#include "Vela/Graphics/Pass.hpp"

#include <memory>

namespace vela::backend
{
    class RenderGraphImpl;

    class PassAdapter : public Pass
    {
    public:
        PassAdapter(std::unique_ptr<graphics::Pass> pass, RenderGraphImpl& renderGraph);

        PassDeclaration declare() const override;
        void record(const PassContext& passContext) override;

        ~PassAdapter() override = default;

    private:
        std::unique_ptr<graphics::Pass> m_pass;
        RenderGraphImpl& m_renderGraph;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PASS_ADAPTER_HPP
