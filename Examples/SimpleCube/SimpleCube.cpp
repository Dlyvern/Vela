#include <Vela/Core/Context.hpp>
#include <Vela/Core/Window.hpp>
#include <Vela/Windowing/GLFWWindowBackend.hpp>
#include <Vela/Graphics/Mesh.hpp>
#include <Vela/Utility/Resources.hpp>
#include <Vela/Graphics/Material.hpp>
#include <Vela/Graphics/Texture.hpp>
#include <Vela/Graphics/RenderGraph.hpp>
#include <Vela/Scene/Camera.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <vector>

int main()
{
    vela::windowing::GLFWWindowBackend backend;
    vela::core::Window window(backend, 800, 600, "SimpleCube");
    vela::core::Context ctx(backend);
    ctx.createSurfaceFor(window);
    vela::graphics::RenderGraph renderGraph(ctx);

    std::vector<vela::graphics::SpriteVertex> verts = 
    {
        {{ 0.0f, -0.5f}, {0.5f, 0.0f}},
        {{ 0.5f,  0.5f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f}, {0.0f, 1.0f}},
    };

    static const std::vector<vela::graphics::StaticVertex> cubeVerts =
    {
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},

        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},

        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},

        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},

        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},

        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}},

        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
    };

    vela::graphics::Mesh triangle(ctx, verts);
    vela::graphics::Mesh cube(ctx, cubeVerts);

    vela::graphics::Material material(ctx, vela::utilities::resources::find("shaders/static_shader.vert.spv").string(), vela::utilities::resources::find("shaders/static_shader.frag.spv").string());
    vela::graphics::Texture texture(ctx, vela::utilities::resources::find("VelixV.png").string());
    material.setAlbedoTexture(texture);

    vela::scene::Camera camera;
    camera.setAspect(800.0f / 600.0f);
    camera.setPosition({0.0f, 0.0f, 3.0f});
    
    while(window.isOpen())
    {
        window.pollEvents();
        float t = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(1,0,1));
        material.setMVP( camera.getViewMatrix(), camera.getProjectionMatrix());

        renderGraph.beginFrame();

        renderGraph.beginPresentPass();
        // renderGraph.draw(triangle, material);
        renderGraph.draw(cube, material, model);
        renderGraph.endRenderPass();

        renderGraph.endFrame();
    }
}