#ifndef VELA_BACKEND_SAMPLER_CACHE_HPP
#define VELA_BACKEND_SAMPLER_CACHE_HPP

#include "volk.h"

#include <cstdint>
#include <unordered_map>

#include "Vela/Graphics/Sampler.hpp"
#include "Vela/Core/DeviceInfo.hpp"

namespace vela::backend
{
    class SamplerCache
    {
    public:
        SamplerCache(VkDevice device, const core::DeviceInfo& deviceInfo);
        VkSampler getSampler(const graphics::SamplerDescription& samplerDescription);
        void clear();
        ~SamplerCache();
    private:
        const core::DeviceInfo& m_deviceInfo;
        VkDevice m_device{VK_NULL_HANDLE};
        std::unordered_map<uint64_t, VkSampler> m_samplers;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_SAMPLER_CACHE_HPP