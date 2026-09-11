#include "Vela/Graphics/ComputeProgram.hpp"

#include "ComputeProgramImpl.hpp"

namespace vela::graphics
{
    Result<ComputeProgram> ComputeProgram::create(core::Context& ctx, std::span<const uint32_t> computeShader)
    {
        try
        {
            return ComputeProgram(ctx, computeShader);
        }
        catch(const std::exception& ex)
        {
            return Error{ErrorCode::AllocationFailed, "Failed to create compute program"};
        }
    }

    ComputeProgram::~ComputeProgram() = default;

    backend::ComputeProgramImpl* ComputeProgram::impl() const
    {
        return m_impl.get();
    }

    ComputeProgram::ComputeProgram(ComputeProgram&&) noexcept = default;
    ComputeProgram& ComputeProgram::operator=(ComputeProgram&&) noexcept = default;

    ComputeProgram::ComputeProgram(core::Context& ctx, std::span<const uint32_t> computeShader)
    {
        m_impl = std::make_unique<backend::ComputeProgramImpl>(ctx, computeShader);
    }

} //namespace vela::graphics