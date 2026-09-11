#include "LittleEngine/FlyCamera.hpp"
#include "LittleEngine/ImGuiLayer.hpp"
#include "LittleEngine/Model.hpp"

#include "Vela/Assets/Resources.hpp"
#include "Vela/Assets/Shader.hpp"
#include "Vela/Builtins/ScenePass.hpp"
#include "Vela/Core/Context.hpp"
#include "Vela/Core/Window.hpp"
#include "Vela/Graphics/RenderGraph.hpp"
#include "Vela/Graphics/RenderScene.hpp"

#include <chrono>
#include <iostream>
#include <memory>

namespace
{
    int fail(const vela::Error& error)
    {
        std::cerr << "Vela error " << static_cast<uint32_t>(error.code) << ": " << error.message << '\n';
        return 1;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: LittleEngine <path to .gltf>\n";
        return 1;
    }

    auto windowResult = vela::core::Window::create({.title = "LittleEngine", .width = 1280, .height = 720});

    if (!windowResult)
        return fail(windowResult.error());

    vela::core::Window window = std::move(windowResult).value();

    vela::core::ContextPreferences contextPreferences{};
    contextPreferences.preferredVSync = vela::core::VSync::Fast;

    auto contextResult = vela::core::Context::create(window, contextPreferences);

    if (!contextResult)
        return fail(contextResult.error());

    vela::core::Context context = std::move(contextResult).value();

    if (auto attached = context.attach(window); !attached)
        return fail(attached.error());

    const vela::core::SwapchainInfo swapchain = context.getSwapchainInfo();

    std::cout << "swapchain " << swapchain.width << 'x' << swapchain.height
              << " | images " << swapchain.imageCount
              << " | vsync requested " << vela::core::toString(swapchain.requestedVSync)
              << ", got " << vela::core::toString(swapchain.actualVSync) << '\n';

    auto litVertResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/lit.vert.spv").string());
    auto litFragResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/lit.frag.spv").string());
    auto uiVertResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/imgui.vert.spv").string());
    auto uiFragResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/imgui.frag.spv").string());

    if (!litVertResult) return fail(litVertResult.error());
    if (!litFragResult) return fail(litFragResult.error());
    if (!uiVertResult) return fail(uiVertResult.error());
    if (!uiFragResult) return fail(uiFragResult.error());

    const std::vector<uint32_t> litVert = std::move(litVertResult).value();
    const std::vector<uint32_t> litFrag = std::move(litFragResult).value();
    const std::vector<uint32_t> uiVert = std::move(uiVertResult).value();
    const std::vector<uint32_t> uiFrag = std::move(uiFragResult).value();

    const auto loadStart = std::chrono::steady_clock::now();

    auto modelResult = little::loadModel(context, argv[1], litVert, litFrag);

    if (!modelResult)
        return fail(modelResult.error());

    little::Model model = std::move(modelResult).value();

    const double loadMilliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - loadStart).count();

    std::cout << "load " << loadMilliseconds << " ms"
              << " | parse " << model.parseMilliseconds << " ms"
              << " | images " << model.imageMilliseconds << " ms"
              << " | upload " << model.uploadMilliseconds << " ms\n";

    std::cout << "meshes " << model.meshes.size()
              << " | materials " << model.materials.size()
              << " | textures " << model.textures.size()
              << " | instances " << model.instances.size() << '\n';

    little::ImGuiLayer ui;

    if (auto ready = ui.initialise(context, uiVert, uiFrag); !ready)
        return fail(ready.error());

    vela::graphics::RenderScene scene;

    for (const little::Instance& instance : model.instances)
        scene.add(model.meshes[instance.meshIndex],
            model.materials[std::min(instance.materialIndex, model.materials.size() - 1)],
            instance.transform);

    auto graphResult = vela::graphics::RenderGraph::create(context);

    if (!graphResult)
        return fail(graphResult.error());

    vela::graphics::RenderGraph graph = std::move(graphResult).value();

    if (auto added = graph.addPass("scene", std::make_unique<vela::builtins::ScenePass>(scene)); !added)
        return fail(added.error());

    if (auto added = graph.addPass("ui", ui.createPass("color")); !added)
        return fail(added.error());

    graph.setPresentSource("color");

    little::FlyCamera camera;
    camera.frame(model.bounds.center(), model.bounds.diagonal());

    auto lastTime = std::chrono::steady_clock::now();
    vela::core::Event event{};

    bool fullscreen = false;

    auto lastStatPrint = std::chrono::steady_clock::now();
    uint32_t statFrames = 0;
    float statCpuMs = 0.0f;
    float statGpuMs = 0.0f;

    while (window.isOpen())
    {
        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        window.pollEvents();

        while (window.popEvent(event))
        {
            ui.handleEvent(event);

            if (event.type == vela::core::EventType::KeyReleased && event.key == vela::core::Key::F11)
            {
                fullscreen = !fullscreen;

                window.setMode(fullscreen
                    ? vela::core::WindowMode::ExclusiveFullscreen
                    : vela::core::WindowMode::Windowed);
            }
        }

        int width = 0;
        int height = 0;

        window.getFramebufferSize(width, height);

        if (height > 0)
            camera.camera().setAspect(static_cast<float>(width) / static_cast<float>(height));

        camera.update(window, deltaTime, !ui.wantsKeyboard() && !ui.wantsMouse());

        ui.beginFrame(window, deltaTime);

        const vela::graphics::FrameStats stats = graph.getFrameStats();
        const vela::core::MemoryStats memory = context.getMemoryStats();

        ++statFrames;
        statCpuMs += stats.cpuFrameMs;
        statGpuMs += stats.gpuTimingValid ? stats.gpuFrameMs : 0.0f;

        if (const auto elapsed = now - lastStatPrint; elapsed >= std::chrono::seconds(1))
        {
            const float seconds = std::chrono::duration<float>(elapsed).count();

            std::cout << "fps " << static_cast<uint32_t>(statFrames / seconds)
                      << " | cpu " << statCpuMs / static_cast<float>(statFrames) << " ms"
                      << " | gpu " << statGpuMs / static_cast<float>(statFrames) << " ms"
                      << " | draws " << model.instances.size()
                      << " | vram " << memory.deviceBytesUsed / (1024 * 1024) << " / "
                      << memory.deviceBytesBudget / (1024 * 1024) << " MB"
                      << " | allocations " << memory.allocationCount
                      << " | validation " << context.getValidationMessageCount() << '\n';

            statFrames = 0;
            statCpuMs = 0.0f;
            statGpuMs = 0.0f;
            lastStatPrint = now;
        }

        ImGui::Begin("LittleEngine");
        ImGui::Text("%s", context.getDeviceInfo().name.c_str());
        ImGui::Separator();
        ImGui::Text("cpu %.2f ms", stats.cpuFrameMs);
        ImGui::Text("gpu %.2f ms", stats.gpuTimingValid ? stats.gpuFrameMs : 0.0f);
        ImGui::Text("fps %.0f", deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f);
        ImGui::Separator();
        ImGui::Text("draws     %zu", model.instances.size());
        ImGui::Text("meshes    %zu", model.meshes.size());
        ImGui::Text("materials %zu", model.materials.size());
        ImGui::Text("textures  %zu", model.textures.size());
        ImGui::Separator();
        ImGui::Text("vram %llu / %llu MB",
            static_cast<unsigned long long>(memory.deviceBytesUsed / (1024 * 1024)),
            static_cast<unsigned long long>(memory.deviceBytesBudget / (1024 * 1024)));
        ImGui::Text("allocations %u", memory.allocationCount);
        ImGui::Separator();
        ImGui::Text("load    %.0f ms", loadMilliseconds);
        ImGui::Text("  parse  %.0f ms", model.parseMilliseconds);
        ImGui::Text("  images %.0f ms", model.imageMilliseconds);
        ImGui::Text("  upload %.0f ms", model.uploadMilliseconds);
        ImGui::Separator();

        float speed = camera.speed();

        if (ImGui::SliderFloat("speed", &speed, 1.0f, model.bounds.diagonal()))
            camera.setSpeed(speed);

        ImGui::Text("validation messages %u", context.getValidationMessageCount());
        ImGui::End();

        ImGui::Render();

        graph.setView(camera.camera().getViewMatrix(), camera.camera().getProjectionMatrix());

        if (auto frame = graph.execute(); !frame)
            return fail(frame.error());
    }

    context.waitIdle();

    return 0;
}
