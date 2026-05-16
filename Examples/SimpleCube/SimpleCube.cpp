#include <Vela/Core/Context.hpp>
#include <Vela/Core/Window.hpp>
#include <Vela/Windowing/GLFWWindowBackend.hpp>
#include <Vela/Graphics/Mesh.hpp>
#include <Vela/Utility/Resources.hpp>
#include <Vela/Graphics/Material.hpp>
#include <Vela/Graphics/Texture.hpp>
#include <Vela/Graphics/RenderGraph.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

int main()
{
    vela::windowing::GLFWWindowBackend backend;
    vela::core::Window window(backend, 800, 600, "SimpleCube");
    vela::core::Context ctx(backend);
    ctx.createSurfaceFor(window);
    vela::graphics::RenderGraph renderGraph(ctx);

    std::vector<vela::graphics::Vertex> verts = 
    {
        {{ 0.0f, -0.5f}, {0.5f, 0.0f}},
        {{ 0.5f,  0.5f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f}, {0.0f, 1.0f}},
    };

    vela::graphics::Mesh triangle(ctx, verts);
    vela::graphics::Material material(ctx, vela::utilities::resources::find("shaders/triangle.vert.spv").string(), vela::utilities::resources::find("shaders/triangle.frag.spv").string());
    vela::graphics::Texture texture(ctx, vela::utilities::resources::find("VelixV.png").string());
    material.setAlbedoTexture(texture);
    
    while(window.isOpen())
    {
        window.pollEvents();
        float t = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0,0,1));
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        material.setMVP(model, view, projection);

        renderGraph.beginFrame();

        renderGraph.beginPresentPass();
        renderGraph.draw(triangle, material);
        renderGraph.endRenderPass();

        renderGraph.endFrame();
    }
}