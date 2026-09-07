#ifndef VELA_CORE_MEMORY_STATS_HPP
#define VELA_CORE_MEMORY_STATS_HPP

#include <cstdint>

namespace vela::core
{
    struct MemoryStats
    {
        uint64_t deviceBytesUsed{0};
        uint64_t deviceBytesBudget{0};
        uint64_t deviceBytesAllocated{0};
        uint32_t allocationCount{0};
        bool budgetFromDriver{false};
    };

} //namespace vela::core

#endif //VELA_CORE_MEMORY_STATS_HPP