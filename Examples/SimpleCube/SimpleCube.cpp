#include <Vela/Core/Context.hpp>
#include <Vela/Core/Window.hpp>
#include <Vela/Graphics/Mesh.hpp>
#include <Vela/Assets/Image.hpp>
#include <Vela/Assets/Shader.hpp>
#include <Vela/Assets/Resources.hpp>
#include <Vela/Graphics/Material.hpp>
#include <Vela/Graphics/Texture.hpp>
#include <Vela/Graphics/RenderGraph.hpp>
#include <Vela/Scene/Camera.hpp>
#include <chrono>
#include <iostream>
#include <vector>

#include "Vela/Math/Math.hpp"

int main()
{
    auto fail = [](const vela::Error& error)
    {
        std::cerr << "Vela error " << static_cast<uint32_t>(error.code) << ": " << error.message << '\n';
        return 1;
    };

    auto windowResult = vela::core::Window::create({.title = "SimpleCube", .width = 800, .height = 600});

    if (!windowResult)
        return fail(windowResult.error());

    vela::core::Window window = std::move(windowResult).value();

    vela::core::ContextPreferences contextPreferences{};
    contextPreferences.preferredGpu = vela::core::GpuPreference::Discrete;

    auto contextResult = vela::core::Context::create(window, contextPreferences);

    if (!contextResult)
        return fail(contextResult.error());

    vela::core::Context ctx = std::move(contextResult).value();

    if (auto attached = ctx.attach(window); !attached)
        return fail(attached.error());

    auto renderGraphResult = vela::graphics::RenderGraph::create(ctx);

    if (!renderGraphResult)
        return fail(renderGraphResult.error());

    vela::graphics::RenderGraph renderGraph = std::move(renderGraphResult).value();

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

    auto triangleResult = vela::graphics::Mesh::create(ctx, verts);
    auto cubeResult = vela::graphics::Mesh::create(ctx, cubeVerts);

    if (!triangleResult) return fail(triangleResult.error());
    if (!cubeResult) return fail(cubeResult.error());

    vela::graphics::Mesh triangle = std::move(triangleResult).value();
    vela::graphics::Mesh cube = std::move(cubeResult).value();

    auto staticVertResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/static_shader.vert.spv").string());
    auto staticFragResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/static_shader.frag.spv").string());
    auto spriteVertResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/sprite_shader.vert.spv").string());
    auto spriteFragResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/sprite_shader.frag.spv").string());

    if (!staticVertResult) return fail(staticVertResult.error());
    if (!staticFragResult) return fail(staticFragResult.error());
    if (!spriteVertResult) return fail(spriteVertResult.error());
    if (!spriteFragResult) return fail(spriteFragResult.error());

    const std::vector<uint32_t> staticVert = std::move(staticVertResult).value();
    const std::vector<uint32_t> staticFrag = std::move(staticFragResult).value();
    const std::vector<uint32_t> spriteVert = std::move(spriteVertResult).value();
    const std::vector<uint32_t> spriteFrag = std::move(spriteFragResult).value();

    auto materialResult = vela::graphics::Material::create(ctx,
    {
        .vertexShader = staticVert,
        .fragmentShader = staticFrag,
        .vertexLayout = vela::graphics::StaticVertex::layout(),
        .textureCount = 1
    });

    if (!materialResult)
        return fail(materialResult.error());

    vela::graphics::Material material = std::move(materialResult).value();

    auto spriteMaterialResult = vela::graphics::Material::create(ctx,
    {
        .vertexShader = spriteVert,
        .fragmentShader = spriteFrag,
        .vertexLayout = vela::graphics::SpriteVertex::layout(),
        .textureCount = 1
    });

    if (!spriteMaterialResult)
        return fail(spriteMaterialResult.error());

    vela::graphics::Material spriteMaterial = std::move(spriteMaterialResult).value();

    auto imageResult = vela::assets::Image::load(vela::assets::resources::find("VelixV.png").string());

    if (!imageResult)
        return fail(imageResult.error());

    vela::assets::Image image = std::move(imageResult).value();

    auto textureResult = vela::graphics::Texture::create(ctx, image.data());

    if (!textureResult)
        return fail(textureResult.error());

    vela::graphics::Texture texture = std::move(textureResult).value();

    if (auto bound = material.setTexture(0, texture); !bound)
        return fail(bound.error());

    if (auto bound = spriteMaterial.setTexture(0, texture); !bound)
        return fail(bound.error());

    vela::scene::Camera camera;
    camera.setAspect(800.0f / 600.0f);
    camera.setPosition({0.0f, 0.0f, 3.0f});
    
    while(window.isOpen())
    {
        window.pollEvents();
        float t = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
        vela::math::Mat4 model = vela::math::rotate(vela::math::Mat4(1.0f), t, vela::math::Vector3f(1,0,1));

        renderGraph.updatePerViewDescriptors(camera.getViewMatrix(), camera.getProjectionMatrix());

        vela::math::Mat4 spriteModel = vela::math::translate(vela::math::Mat4(1.0f), vela::math::Vector3f(-1.5f, 0.0f, 0.0f));

        renderGraph.beginFrame();

        renderGraph.beginPass("scene");
        renderGraph.draw(cube, material, model);
        renderGraph.draw(triangle, spriteMaterial, spriteModel);
        renderGraph.endPass();

        renderGraph.beginPass("present");
        renderGraph.endPass();

        renderGraph.endFrame();
    }

    ctx.waitIdle();

    return 0;
}