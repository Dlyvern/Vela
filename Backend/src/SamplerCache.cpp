#include "SamplerCache.hpp"

#include "Formats.hpp"

#include <stdexcept>
#include <iostream>

constexpr uint64_t k_fnvOffset = 1469598103934665603ull;
constexpr uint64_t k_fnvPrime  = 1099511628211ull;

uint64_t hashBytes(const void* data, size_t size, uint64_t seed)
{
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint64_t hash = seed;

    for (size_t i = 0; i < size; ++i)
    {
        hash ^= bytes[i];
        hash *= k_fnvPrime;
    }

    return hash;
}

template<typename T>
uint64_t hashValue(const T& value, uint64_t seed)
{
    static_assert(std::is_trivially_copyable_v<T>);
    return hashBytes(&value, sizeof(T), seed);
}

namespace vela::backend
{
    SamplerCache::SamplerCache(VkDevice device, const core::DeviceInfo& deviceInfo) : m_device(device),
    m_deviceInfo(deviceInfo)
    {

    }

    VkSampler SamplerCache::getSampler(const graphics::SamplerDescription& samplerDescription)
    {
        uint64_t hash = k_fnvOffset;

        hash = hashValue(samplerDescription.minFilter, hash);
        hash = hashValue(samplerDescription.magFilter, hash);
        hash = hashValue(samplerDescription.addressU, hash);
        hash = hashValue(samplerDescription.addressV, hash);
        hash = hashValue(samplerDescription.addressW, hash);
        hash = hashValue(samplerDescription.mipMode, hash);
        hash = hashValue(samplerDescription.minLod, hash);
        hash = hashValue(samplerDescription.maxLod, hash);
        hash = hashValue(samplerDescription.anisotropy, hash);
        hash = hashValue(samplerDescription.compareOp, hash);

        if(auto it = m_samplers.find(hash); it != m_samplers.end())
            return it->second;

        VkSamplerCreateInfo samplerCI{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerCI.magFilter = toVkFilter(samplerDescription.magFilter);
        samplerCI.minFilter = toVkFilter(samplerDescription.minFilter);
        samplerCI.addressModeU = toVkAddressMode(samplerDescription.addressU);
        samplerCI.addressModeV = toVkAddressMode(samplerDescription.addressV);
        samplerCI.addressModeW = toVkAddressMode(samplerDescription.addressW);
        samplerCI.maxLod = samplerDescription.maxLod;
        samplerCI.minLod = samplerDescription.minLod;
        samplerCI.compareEnable = samplerDescription.compareOp != graphics::DepthCompare::None;

        if(samplerDescription.compareOp != graphics::DepthCompare::None)
            samplerCI.compareOp = toVkCompare(samplerDescription.compareOp);

        if(m_deviceInfo.enabled(core::Feature::SamplerAnisotropy))
        {
            samplerCI.anisotropyEnable = true;
            samplerCI.maxAnisotropy = samplerDescription.anisotropy;
        }
        else
            samplerCI.maxAnisotropy = 1.0f;

        VkSampler sampler;

        if(VkResult result = vkCreateSampler(m_device, &samplerCI, nullptr, &sampler); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create sampler");

        m_samplers[hash] = sampler;

        std::cout << "New sampler was created with " << hash << " key\n";

        return sampler;
    }

    void SamplerCache::clear()
    {
        for(const auto& [_, sampler] : m_samplers)
            vkDestroySampler(m_device, sampler, nullptr);

        m_samplers.clear();
    }

    SamplerCache::~SamplerCache()
    {
        clear();
    }
} //namespace vela::backend