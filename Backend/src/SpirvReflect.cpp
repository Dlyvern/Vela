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

    constexpr uint32_t k_opTypeInt = 21;
    constexpr uint32_t k_opTypeFloat = 22;
    constexpr uint32_t k_opTypeVector = 23;
    constexpr uint32_t k_opTypeMatrix = 24;
    constexpr uint32_t k_opTypeArray = 28;
    constexpr uint32_t k_opTypeStruct = 30;
    constexpr uint32_t k_opTypePointer = 32;
    constexpr uint32_t k_opConstant = 43;
    constexpr uint32_t k_opVariable = 59;
    constexpr uint32_t k_opMemberDecorate = 72;

    constexpr uint32_t k_decorationOffset = 35;
    constexpr uint32_t k_storageClassPushConstant = 9;

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

    uint32_t pushConstantBlockSize(std::span<const uint32_t> code)
    {
        if (code.size() < k_headerWords || code[0] != k_spirvMagic)
            return 0;

        std::unordered_map<uint32_t, uint32_t> typeSizes;
        std::unordered_map<uint32_t, uint32_t> constants;
        std::unordered_map<uint32_t, uint32_t> pointeeTypes;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> memberOffsets;

        uint32_t pushConstantPointer = 0;
        bool hasPushConstant = false;

        auto sizeOf = [&typeSizes](uint32_t type) -> uint32_t
        {
            const auto it = typeSizes.find(type);
            return it == typeSizes.end() ? 0 : it->second;
        };

        size_t offset = k_headerWords;

        while (offset < code.size())
        {
            const uint32_t instruction = code[offset];
            const uint32_t wordCount = instruction >> 16;
            const uint32_t opcode = instruction & 0xFFFFu;

            if (wordCount == 0 || offset + wordCount > code.size())
                break;

            switch (opcode)
            {
                case k_opMemberDecorate:
                    if (wordCount >= 5 && code[offset + 3] == k_decorationOffset)
                        memberOffsets[code[offset + 1]][code[offset + 2]] = code[offset + 4];
                    break;

                case k_opTypeInt:
                case k_opTypeFloat:
                    if (wordCount >= 3)
                        typeSizes[code[offset + 1]] = code[offset + 2] / 8;
                    break;

                case k_opTypeVector:
                case k_opTypeMatrix:
                    if (wordCount >= 4)
                        typeSizes[code[offset + 1]] = sizeOf(code[offset + 2]) * code[offset + 3];
                    break;

                case k_opTypeArray:
                    if (wordCount >= 4)
                    {
                        const auto length = constants.find(code[offset + 3]);

                        if (length != constants.end())
                            typeSizes[code[offset + 1]] = sizeOf(code[offset + 2]) * length->second;
                    }
                    break;

                case k_opTypeStruct:
                    if (wordCount >= 2)
                    {
                        const uint32_t structType = code[offset + 1];
                        const auto& offsets = memberOffsets[structType];

                        uint32_t structSize = 0;

                        for (uint32_t member = 0; member + 2 < wordCount; ++member)
                        {
                            const auto memberOffset = offsets.find(member);

                            if (memberOffset == offsets.end())
                                continue;

                            structSize = std::max(structSize,
                                memberOffset->second + sizeOf(code[offset + 2 + member]));
                        }

                        typeSizes[structType] = structSize;
                    }
                    break;

                case k_opTypePointer:
                    if (wordCount >= 4 && code[offset + 2] == k_storageClassPushConstant)
                        pointeeTypes[code[offset + 1]] = code[offset + 3];
                    break;

                case k_opConstant:
                    if (wordCount >= 4)
                        constants[code[offset + 2]] = code[offset + 3];
                    break;

                case k_opVariable:
                    if (wordCount >= 4 && code[offset + 3] == k_storageClassPushConstant)
                    {
                        pushConstantPointer = code[offset + 1];
                        hasPushConstant = true;
                    }
                    break;

                default:
                    break;
            }

            offset += wordCount;
        }

        if (!hasPushConstant)
            return 0;

        const auto pointee = pointeeTypes.find(pushConstantPointer);

        if (pointee == pointeeTypes.end())
            return 0;

        return sizeOf(pointee->second);
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

    uint32_t reflectPushConstantSize(std::span<const uint32_t> shader)
    {
        return pushConstantBlockSize(shader);
    }
} //namespace vela::backend
