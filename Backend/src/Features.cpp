#include "Features.hpp"

#include <cstring>
#include <vector>

namespace vela::backend
{
    namespace
    {
        constexpr const char* k_rayTracingExtensions[] = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
        };

        constexpr const char* k_meshShaderExtensions[] = {
            VK_EXT_MESH_SHADER_EXTENSION_NAME
        };

        constexpr core::Feature k_allFeatures[] = {
            core::Feature::SamplerAnisotropy,
            core::Feature::FillModeNonSolid,
            core::Feature::WideLines,
            core::Feature::Bindless,
            core::Feature::RayTracing,
            core::Feature::MeshShaders
        };

        bool hasDeviceExtension(VkPhysicalDevice physicalDevice, const char* extensionName)
        {
            uint32_t extensionCount{0};
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> extensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

            for (const VkExtensionProperties& extension : extensions)
                if (std::strcmp(extension.extensionName, extensionName) == 0)
                    return true;

            return false;
        }
    }

    std::span<const char* const> featureExtensions(core::Feature feature)
    {
        switch (feature)
        {
            case core::Feature::RayTracing:  return k_rayTracingExtensions;
            case core::Feature::MeshShaders: return k_meshShaderExtensions;

            case core::Feature::SamplerAnisotropy:
            case core::Feature::FillModeNonSolid:
            case core::Feature::WideLines:
            case core::Feature::Bindless:
                return {};
        }

        return {};
    }

    std::span<const core::Feature> allFeatures()
    {
        return k_allFeatures;
    }

    void FeatureChain::link(std::span<const core::Feature> features)
    {
        bool rayTracing{false};
        bool meshShaders{false};

        for (core::Feature feature : features)
        {
            if (feature == core::Feature::RayTracing)
                rayTracing = true;

            if (feature == core::Feature::MeshShaders)
                meshShaders = true;
        }

        features2.pNext = &features13;
        features13.pNext = &features12;

        void** next = &features12.pNext;

        if (rayTracing)
        {
            *next = &accelerationStructure;
            next = &accelerationStructure.pNext;

            *next = &rayTracingPipeline;
            next = &rayTracingPipeline.pNext;
        }

        if (meshShaders)
        {
            *next = &meshShader;
            next = &meshShader.pNext;
        }

        *next = nullptr;
    }

    void FeatureChain::enable(core::Feature feature)
    {
        switch (feature)
        {
            case core::Feature::SamplerAnisotropy:
                features2.features.samplerAnisotropy = VK_TRUE;
                break;

            case core::Feature::FillModeNonSolid:
                features2.features.fillModeNonSolid = VK_TRUE;
                break;

            case core::Feature::WideLines:
                features2.features.wideLines = VK_TRUE;
                break;

            case core::Feature::Bindless:
                features12.runtimeDescriptorArray = VK_TRUE;
                features12.descriptorBindingPartiallyBound = VK_TRUE;
                features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
                features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
                break;

            case core::Feature::RayTracing:
                features12.bufferDeviceAddress = VK_TRUE;
                accelerationStructure.accelerationStructure = VK_TRUE;
                rayTracingPipeline.rayTracingPipeline = VK_TRUE;
                break;

            case core::Feature::MeshShaders:
                meshShader.meshShader = VK_TRUE;
                break;
        }
    }

    bool FeatureChain::has(core::Feature feature) const
    {
        switch (feature)
        {
            case core::Feature::SamplerAnisotropy:
                return features2.features.samplerAnisotropy == VK_TRUE;

            case core::Feature::FillModeNonSolid:
                return features2.features.fillModeNonSolid == VK_TRUE;

            case core::Feature::WideLines:
                return features2.features.wideLines == VK_TRUE;

            case core::Feature::Bindless:
                return features12.runtimeDescriptorArray == VK_TRUE
                    && features12.descriptorBindingPartiallyBound == VK_TRUE
                    && features12.shaderSampledImageArrayNonUniformIndexing == VK_TRUE
                    && features12.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE;

            case core::Feature::RayTracing:
                return features12.bufferDeviceAddress == VK_TRUE
                    && accelerationStructure.accelerationStructure == VK_TRUE
                    && rayTracingPipeline.rayTracingPipeline == VK_TRUE;

            case core::Feature::MeshShaders:
                return meshShader.meshShader == VK_TRUE;
        }

        return false;
    }

    bool deviceSupportsFeature(VkPhysicalDevice physicalDevice, core::Feature feature)
    {
        for (const char* extensionName : featureExtensions(feature))
            if (!hasDeviceExtension(physicalDevice, extensionName))
                return false;

        FeatureChain chain{};
        chain.link(std::span<const core::Feature>(&feature, 1));

        vkGetPhysicalDeviceFeatures2(physicalDevice, &chain.features2);

        return chain.has(feature);
    }
} //namespace vela::backend
