#ifndef VELA_ASSETS_SHADER_HPP
#define VELA_ASSETS_SHADER_HPP

#include "Vela/Result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace vela::assets
{
    Result<std::vector<uint32_t>> loadSpirv(const std::string& path);
} //namespace vela::assets

#endif //VELA_ASSETS_SHADER_HPP
