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
    vela::core::Window window(backend, {.title = "SimpleCube", .width = 800, .height = 600});

    vela::core::ContextPreferences contextPreferences{};
    contextPreferences.preferedGpuType = vela::core::GPUDeviceType::eDISCRETE;

    vela::core::Context ctx(backend, contextPreferences);
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

    vela::graphics::Material material(ctx, renderGraph,
    {
        .vertexShaderPath = vela::utilities::resources::find("shaders/static_shader.vert.spv").string(),
        .fragmentShaderPath = vela::utilities::resources::find("shaders/static_shader.frag.spv").string(),
        .vertexLayout = vela::graphics::StaticVertex::layout()
    });

    vela::graphics::Material spriteMaterial(ctx, renderGraph,
    {
        .vertexShaderPath = vela::utilities::resources::find("shaders/sprite_shader.vert.spv").string(),
        .fragmentShaderPath = vela::utilities::resources::find("shaders/sprite_shader.frag.spv").string(),
        .vertexLayout = vela::graphics::SpriteVertex::layout()
    });

    vela::graphics::Texture texture(ctx, vela::utilities::resources::find("VelixV.png").string());
    material.setAlbedoTexture(texture);
    spriteMaterial.setAlbedoTexture(texture);

    vela::scene::Camera camera;
    camera.setAspect(800.0f / 600.0f);
    camera.setPosition({0.0f, 0.0f, 3.0f});
    
    while(window.isOpen())
    {
        window.pollEvents();
        float t = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(1,0,1));

        renderGraph.updatePerViewDescriptors(camera.getViewMatrix(), camera.getProjectionMatrix());

        glm::mat4 spriteModel = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, 0.0f));

        renderGraph.beginFrame();

        renderGraph.beginPresentPass();
        renderGraph.draw(cube, material, model);
        renderGraph.draw(triangle, spriteMaterial, spriteModel);
        renderGraph.endRenderPass();

        renderGraph.endFrame();
    }

    ctx.waitIdle();
}