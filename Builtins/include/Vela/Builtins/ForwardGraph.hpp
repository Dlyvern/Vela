#ifndef VELA_BUILTINS_FORWARD_GRAPH_HPP
#define VELA_BUILTINS_FORWARD_GRAPH_HPP

#include "Vela/Graphics/RenderGraph.hpp"
#include "Vela/Graphics/RenderScene.hpp"
#include "Vela/Result.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::builtins
{
    [[nodiscard]] Result<graphics::RenderGraph> forwardGraph(core::Context& context,
        const graphics::RenderScene& renderScene);
} //namespace vela::builtins

#endif //VELA_BUILTINS_FORWARD_GRAPH_HPP
