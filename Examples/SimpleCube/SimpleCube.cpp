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
#include <Vela/Builtins/ForwardGraph.hpp>
#include <Vela/Builtins/Shapes.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "Vela/Math/Math.hpp"

void cameraMovement(vela::scene::Camera& camera, float deltaTime, vela::core::Window& window)
{
    constexpr float movementSpeed = 5.0f;
    
    auto position = camera.getPosition();

    if(window.isKeyDown(vela::core::Key::W))
    {
        position.z -= movementSpeed * deltaTime;
    }

    if(window.isKeyDown(vela::core::Key::S))
    {
        position.z += movementSpeed * deltaTime;
    }

    if(window.isKeyDown(vela::core::Key::A))
    {
        position.x -= movementSpeed * deltaTime;
    }

    if(window.isKeyDown(vela::core::Key::D))
    {
        position.x += movementSpeed * deltaTime;
    }

    if(window.isKeyDown(vela::core::Key::Q))
    {
        position.y -= movementSpeed * deltaTime;
    }

    if(window.isKeyDown(vela::core::Key::E))
    {
        position.y += movementSpeed * deltaTime;
    }

    camera.setPosition(position);
    
    constexpr float sensitivity = 0.1f;
    static bool firstMouse = false;
    static double lastX = 0.0;
    static double lastY = 0.0;

    const bool rightMouse = window.isMouseButtonDown(vela::core::MouseButton::Right);

    if (!rightMouse)
    {
        firstMouse = true;
        return;
    }

    double x;
    double y;

    window.getCursorPosition(x, y);

    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
        return;
    }

    const float offsetX =
        static_cast<float>(x - lastX);

    const float offsetY =
        static_cast<float>(y - lastY);

    lastX = x;
    lastY = y;

    camera.setYaw(camera.getYaw() + offsetX * sensitivity);

    float newPitch = camera.getPitch() - offsetY * sensitivity;

    newPitch = std::clamp(newPitch, -89.0f, 89.0f);

    camera.setPitch(newPitch);

    camera.updateCameraVectors();
}

void gamepadCameraMovement(vela::scene::Camera& camera, float deltaTime, vela::core::Window& window)
{
    const std::vector<int> gamepads = window.getConnectedGamepads();

    if (gamepads.empty())
        return;

    const int gamepad = gamepads.front();

    constexpr float movementSpeed = 5.0f;
    constexpr float lookSpeed = 120.0f;

    auto deadzoned = [](float value)
    {
        return std::abs(value) < 0.15f ? 0.0f : value;
    };

    auto position = camera.getPosition();

    position.x += deadzoned(window.getGamepadAxisLeftX(gamepad)) * movementSpeed * deltaTime;
    position.z += deadzoned(window.getGamepadAxisLeftY(gamepad)) * movementSpeed * deltaTime;

    if (window.isGamepadButtonDown(gamepad, vela::core::GamepadButton::LEFT_BUMPER))
        position.y -= movementSpeed * deltaTime;

    if (window.isGamepadButtonDown(gamepad, vela::core::GamepadButton::RIGHT_BUMPER))
        position.y += movementSpeed * deltaTime;

    camera.setPosition(position);

    const float lookX = deadzoned(window.getGamepadAxisRightX(gamepad));
    const float lookY = deadzoned(window.getGamepadAxisRightY(gamepad));

    if (lookX == 0.0f && lookY == 0.0f)
        return;

    camera.setYaw(camera.getYaw() + lookX * lookSpeed * deltaTime);
    camera.setPitch(std::clamp(camera.getPitch() - lookY * lookSpeed * deltaTime, -89.0f, 89.0f));

    camera.updateCameraVectors();
}

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

    const auto triangleData = vela::builtins::shapes2d::triangle();
    const auto cubeData = vela::builtins::shapes3d::cube();

    auto triangleResult = vela::graphics::Mesh::create(ctx, triangleData.vertices, triangleData.indices);
    auto cubeResult = vela::graphics::Mesh::create(ctx, cubeData.vertices, cubeData.indices);

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
        .vertexLayout = vela::graphics::StaticVertex::layout()
    });

    if (!materialResult)
        return fail(materialResult.error());

    vela::graphics::Material material = std::move(materialResult).value();

    auto spriteMaterialResult = vela::graphics::Material::create(ctx,
    {
        .vertexShader = spriteVert,
        .fragmentShader = spriteFrag,
        .vertexLayout = vela::graphics::SpriteVertex::layout()
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

    auto lastStatPrint = std::chrono::steady_clock::now();
    uint32_t statFrames = 0;

    window.setCursorMode(vela::core::CursorMode::Normal);

    bool rotateCube{true};

    static float rotation = 0.0f;
    static auto lastTime = std::chrono::steady_clock::now();

    vela::core::Event event{};

    while(window.isOpen())
    {
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();

        lastTime = currentTime;

        window.pollEvents();

        cameraMovement(camera, deltaTime, window);
        gamepadCameraMovement(camera, deltaTime, window);

        while(window.popEvent(event))
        {
            if(event.type == vela::core::EventType::Resized)
            {
                camera.setAspect((static_cast<float>(event.width) / static_cast<float>(event.height)));
            }

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
            {
                if(event.modifiers.control && event.key == vela::core::Key::Escape)
                    window.close();
            }

            if(event.type == vela::core::EventType::CloseRequested)
            {
                std::cout << "We are all alone in this vulkan journey\n";
            }
        }

        if (rotateCube)
        {
            rotation += deltaTime;
        }

        vela::math::Mat4 model = vela::math::rotate(vela::math::Mat4(1.0f), rotation, vela::math::Vector3f(1.0f, 0.0f, 1.0f));

        renderGraph.setView(camera.getViewMatrix(), camera.getProjectionMatrix());

        vela::math::Mat4 spriteModel = vela::math::translate(vela::math::Mat4(1.0f), vela::math::Vector3f(-1.5f, 0.0f, 0.0f));
        renderScene.clear();
        renderScene.add(cube, material, model);
        renderScene.add(triangle, spriteMaterial, spriteModel);

        if (auto frame = renderGraph.execute(); !frame)
            return fail(frame.error());

        ++statFrames;

        if (const auto now = std::chrono::steady_clock::now(); now - lastStatPrint >= std::chrono::seconds(1))
        {
            const float seconds = std::chrono::duration<float>(now - lastStatPrint).count();
            const vela::graphics::FrameStats stats = renderGraph.getFrameStats();
            const vela::core::MemoryStats memory = ctx.getMemoryStats();

            std::cout << "fps " << static_cast<uint32_t>(statFrames / seconds)
                      << " | cpu " << stats.cpuFrameMs << " ms"
                      << " | gpu " << (stats.gpuTimingValid ? std::to_string(stats.gpuFrameMs) + " ms" : "n/a")
                      << " | vela " << memory.deviceBytesAllocated / (1024 * 1024) << " MB"
                      << " | vram " << memory.deviceBytesUsed / (1024 * 1024) << " / "
                      << memory.deviceBytesBudget / (1024 * 1024) << " MB"
                      << " | allocs " << memory.allocationCount
                      << (memory.budgetFromDriver ? "" : " (estimated)")
                      << " | objects " << renderScene.submissions().size() << '\n';

            statFrames = 0;
            lastStatPrint = now;
        }
    }

    ctx.waitIdle();

    return 0;
}