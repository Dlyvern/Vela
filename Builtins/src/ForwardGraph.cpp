#include "Vela/Builtins/ForwardGraph.hpp"

#include "Vela/Builtins/ScenePass.hpp"

#include <memory>

namespace vela::builtins
{
    Result<graphics::RenderGraph> forwardGraph(core::Context& context,
        const graphics::RenderScene& renderScene)
    {
        return forwardGraph(context, renderScene, ScenePassOptions{});
    }

    Result<graphics::RenderGraph> forwardGraph(core::Context& context,
        const graphics::RenderScene& renderScene, const ScenePassOptions& options)
    {
        auto graphResult = graphics::RenderGraph::create(context);

        if (!graphResult)
            return graphResult.error();

        graphics::RenderGraph graph = std::move(graphResult).value();

        if (auto added = graph.addPass("scene", std::make_unique<ScenePass>(renderScene, options)); !added)
            return added.error();

        graph.setPresentSource(k_sceneColorAttachment);

        return std::move(graph);
    }
} //namespace vela::builtins
