#ifndef VELA_GRAPHICS_COMPUTE_PROGRAM_HPP
#define VELA_GRAPHICS_COMPUTE_PROGRAM_HPP

#include "Vela/Result.hpp"

#include <memory>
#include <span>

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class ComputeProgramImpl;
} //namespace vela::backend

namespace vela::graphics
{
    class ComputeProgram
    {
    public:
        static Result<ComputeProgram> create(core::Context& ctx, std::span<const uint32_t> computeShader);
        ~ComputeProgram();

        ComputeProgram(ComputeProgram&&) noexcept;
        ComputeProgram& operator=(ComputeProgram&&) noexcept;

        ComputeProgram(const ComputeProgram&) = delete;
        ComputeProgram& operator=(const ComputeProgram&) = delete;

        backend::ComputeProgramImpl* impl() const;

    private:
        ComputeProgram(core::Context& ctx, std::span<const uint32_t> computeShader);

        std::unique_ptr<backend::ComputeProgramImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_COMPUTE_PROGRAM_HPP