#ifndef VELA_BACKEND_COMPUTE_PASS_ADAPTER_HPP
#define VELA_BACKEND_COMPUTE_PASS_ADAPTER_HPP

#include "Pass.hpp"

#include "Vela/Graphics/Pass.hpp"

#include <memory>

namespace vela::backend
{
    class RenderGraphImpl;

    class ComputePassAdapter : public Pass
    {
    public:
        ComputePassAdapter(std::unique_ptr<graphics::ComputePass> pass, RenderGraphImpl& renderGraph);

        PassDeclaration declare() const override;
        void record(const PassContext& passContext) override;
        PassKind kind() const override;

        ~ComputePassAdapter() override = default;

    private:
        std::unique_ptr<graphics::ComputePass> m_pass;
        RenderGraphImpl& m_renderGraph;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_COMPUTE_PASS_ADAPTER_HPP
