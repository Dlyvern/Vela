#include <Vela/Core/Context.hpp>
#include <Vela/Core/Window.hpp>
#include <Vela/Graphics/Mesh.hpp>
#include <Vela/Assets/Image.hpp>
#include <Vela/Assets/Shader.hpp>
#include <Vela/Assets/Resources.hpp>
#include <Vela/Graphics/Material.hpp>
#include <Vela/Graphics/Texture.hpp>
#include <Vela/Graphics/RenderGraph.hpp>
#include <Vela/Builtins/ForwardGraph.hpp>
#include <Vela/Builtins/Shapes.hpp>

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

    vela::graphics::RenderScene renderScene;

    auto renderGraphResult = vela::builtins::forwardGraph(ctx, renderScene);

    if (!renderGraphResult)
        return fail(renderGraphResult.error());

    vela::graphics::RenderGraph renderGraph = std::move(renderGraphResult).value();

    const auto cubeData = vela::builtins::shapes3d::cube();

    auto cubeResult = vela::graphics::Mesh::create(ctx, cubeData.vertices, cubeData.indices);

    if (!cubeResult)
        return fail(cubeResult.error());

    vela::graphics::Mesh cube = std::move(cubeResult).value();

    auto staticVertResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/static_shader.vert.spv").string());
    auto staticFragResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/static_shader.frag.spv").string());

    if (!staticVertResult) return fail(staticVertResult.error());
    if (!staticFragResult) return fail(staticFragResult.error());

    const std::vector<uint32_t> staticVert = std::move(staticVertResult).value();
    const std::vector<uint32_t> staticFrag = std::move(staticFragResult).value();

    auto materialResult = vela::graphics::Material::create(ctx,
    {
        .vertexShader = staticVert,
        .fragmentShader = staticFrag,
        .vertexLayout = vela::graphics::StaticVertex::layout()
    });

    if (!materialResult)
        return fail(materialResult.error());

    vela::graphics::Material material = std::move(materialResult).value();

    auto imageResult = vela::assets::Image::load(vela::assets::resources::find("Cat.png").string());

    if (!imageResult)
        return fail(imageResult.error());

    vela::assets::Image image = std::move(imageResult).value();

    auto textureResult = vela::graphics::Texture::create(ctx, image.data());

    if (!textureResult)
        return fail(textureResult.error());

    vela::graphics::Texture texture = std::move(textureResult).value();

    if (auto bound = material.setTexture(0, texture); !bound)
        return fail(bound.error());

    const vela::math::Mat4 view = vela::math::lookAt(
        vela::math::Vector3f(0.0f, 1.0f, 3.0f),
        vela::math::Vector3f(0.0f, 0.0f, 0.0f),
        vela::math::Vector3f(0.0f, 1.0f, 0.0f));

    float aspect = 800.0f / 600.0f;

    auto projection = [&aspect]
    {
        vela::math::Mat4 result = vela::math::perspective(vela::math::radians(60.0f), aspect, 0.1f, 100.0f);
        result[1][1] *= -1.0f;

        return result;
    };

    bool rotateCube{true};

    float rotation = 0.0f;
    auto lastTime = std::chrono::steady_clock::now();

    vela::core::Event event{};

    while(window.isOpen())
    {
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();

        lastTime = currentTime;

        window.pollEvents();

        while(window.popEvent(event))
        {
            if(event.type == vela::core::EventType::Resized && event.height > 0)
                aspect = static_cast<float>(event.width) / static_cast<float>(event.height);

            if(event.type == vela::core::EventType::FocusLost)
            {
                rotateCube = false;
                std::cout << "Focus lost\n";
            }

            if(event.type == vela::core::EventType::FocusGained)
            {
                rotateCube = true;
                std::cout << "Focus gained\n";
            }

            if(event.type == vela::core::EventType::KeyPressed)
                if(event.modifiers.control && event.key == vela::core::Key::Escape)
                    window.close();

            if(event.type == vela::core::EventType::CloseRequested)
                std::cout << "We are all alone in this vulkan journey\n";
        }

        if (rotateCube)
            rotation += deltaTime;

        vela::math::Mat4 model = vela::math::rotate(vela::math::Mat4(1.0f), rotation, vela::math::Vector3f(1.0f, 0.0f, 1.0f));

        renderGraph.setView(view, projection());

        renderScene.clear();
        renderScene.add(cube, material, model);

        if (auto frame = renderGraph.execute(); !frame)
            return fail(frame.error());
    }

    ctx.waitIdle();

    return 0;
}