#ifndef VELA_BUILTINS_FORWARD_GRAPH_HPP
#define VELA_BUILTINS_FORWARD_GRAPH_HPP

#include "Vela/Graphics/RenderGraph.hpp"
#include "Vela/Graphics/RenderScene.hpp"
#include "Vela/Builtins/ScenePass.hpp"
#include "Vela/Result.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::builtins
{
    // Creates a graph with one ScenePass (named "scene") that presents k_sceneColorAttachment.
    // context:     the Vela context to render with.
    // renderScene: the scene to draw each frame; must outlive the graph.
    // Returns:     the graph, or an error if it could not be created.
    [[nodiscard]] Result<graphics::RenderGraph> forwardGraph(core::Context& context,
        const graphics::RenderScene& renderScene);

    // Same as above, with the scene pass's clear color and MSAA sample count.
    // options: see ScenePassOptions; an unsupported sample count makes this fail.
    [[nodiscard]] Result<graphics::RenderGraph> forwardGraph(core::Context& context,
        const graphics::RenderScene& renderScene, const ScenePassOptions& options);
} //namespace vela::builtins

#endif //VELA_BUILTINS_FORWARD_GRAPH_HPP
