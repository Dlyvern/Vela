#include "SpirvReflect.hpp"

#include <algorithm>
#include <unordered_map>

namespace
{
    constexpr uint32_t k_spirvMagic = 0x07230203;
    constexpr uint32_t k_headerWords = 5;

    constexpr uint32_t k_opDecorate = 71;

    constexpr uint32_t k_decorationBinding = 33;
    constexpr uint32_t k_decorationDescriptorSet = 34;

    constexpr uint32_t k_materialSet = 1;

    struct Decoration
    {
        bool hasSet{false};
        bool hasBinding{false};
        uint32_t set{0};
        uint32_t binding{0};
    };

    uint32_t highestMaterialBinding(std::span<const uint32_t> code, uint32_t current)
    {
        if (code.size() < k_headerWords || code[0] != k_spirvMagic)
            return current;

        std::unordered_map<uint32_t, Decoration> decorations;

        size_t offset = k_headerWords;

        while (offset < code.size())
        {
            const uint32_t instruction = code[offset];
            const uint32_t wordCount = instruction >> 16;
            const uint32_t opcode = instruction & 0xFFFFu;

            if (wordCount == 0 || offset + wordCount > code.size())
                break;

            if (opcode == k_opDecorate && wordCount >= 4)
            {
                const uint32_t target = code[offset + 1];
                const uint32_t decoration = code[offset + 2];
                const uint32_t operand = code[offset + 3];

                if (decoration == k_decorationDescriptorSet)
                {
                    decorations[target].hasSet = true;
                    decorations[target].set = operand;
                }
                else if (decoration == k_decorationBinding)
                {
                    decorations[target].hasBinding = true;
                    decorations[target].binding = operand;
                }
            }

            offset += wordCount;
        }

        uint32_t highest = current;

        for (const auto& [target, decoration] : decorations)
        {
            if (!decoration.hasSet || !decoration.hasBinding)
                continue;

            if (decoration.set != k_materialSet)
                continue;

            highest = std::max(highest, decoration.binding + 1);
        }

        return highest;
    }
} //namespace

namespace vela::backend
{
    uint32_t reflectTextureCount(std::span<const uint32_t> vertexShader,
        std::span<const uint32_t> fragmentShader)
    {
        uint32_t count = highestMaterialBinding(vertexShader, 0);
        return highestMaterialBinding(fragmentShader, count);
    }
} //namespace vela::backend
