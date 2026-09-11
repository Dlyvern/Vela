#ifndef VELA_FEATURES_HPP
#define VELA_FEATURES_HPP

#include "volk.h"

#include "Vela/Core/Feature.hpp"

#include <span>

namespace vela::backend
{
    struct FeatureChain
    {
        VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructure{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShader{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};

        void link(std::span<const core::Feature> features);
        void enable(core::Feature feature);

        [[nodiscard]] bool has(core::Feature feature) const;
    };

    [[nodiscard]] std::span<const char* const> featureExtensions(core::Feature feature);
    [[nodiscard]] std::span<const core::Feature> allFeatures();
    [[nodiscard]] bool deviceSupportsFeature(VkPhysicalDevice physicalDevice, core::Feature feature);
} //namespace vela::backend

#endif //VELA_FEATURES_HPP
