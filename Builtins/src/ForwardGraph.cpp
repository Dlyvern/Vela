#include "Vela/Builtins/ForwardGraph.hpp"

#include "Vela/Builtins/ScenePass.hpp"

#include <memory>

namespace vela::builtins
{
    Result<graphics::RenderGraph> forwardGraph(core::Context& context,
        const graphics::RenderScene& renderScene)
    {
        auto graphResult = graphics::RenderGraph::create(context);

        if (!graphResult)
            return graphResult.error();

        graphics::RenderGraph graph = std::move(graphResult).value();

        if (auto added = graph.addPass("scene", std::make_unique<ScenePass>(renderScene)); !added)
            return added.error();

        graph.setPresentSource("color");

        return std::move(graph);
    }
} //namespace vela::builtins
