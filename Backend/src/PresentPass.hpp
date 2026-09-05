#ifndef VELA_BACKEND_PRESENT_PASS_HPP
#define VELA_BACKEND_PRESENT_PASS_HPP

#include "Pass.hpp"

namespace vela::backend
{
    class PresentPass : public Pass
    {
    public:
        PresentPass() = default;

        PassDeclaration declare() const override;
        void record(const PassContext& passContext) override;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PRESENT_PASS_HPP
