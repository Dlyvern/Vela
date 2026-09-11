#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>

#include "Vela/Core/Context.hpp"
#include "Vela/Core/Window.hpp"
#include "Vela/Graphics/ComputeProgram.hpp"
#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/RenderGraph.hpp"
#include "Vela/Assets/Resources.hpp"
#include "Vela/Assets/Shader.hpp"
#include "Vela/Math/Math.hpp"

namespace
{
    constexpr uint32_t k_particleCount = 8192;
    constexpr uint32_t k_workGroupSize = 64;
    constexpr uint32_t k_particleStride = 32;
}

class SimulationPass : public vela::graphics::ComputePass
{
public:
    explicit SimulationPass(vela::graphics::ComputeProgram& program) : m_program(program) {}

    void setDeltaTime(float deltaTime) { m_deltaTime = deltaTime; }

    vela::graphics::PassDescription describe() const override
    {
        vela::graphics::PassDescription description{};
        description.bufferOutputs.push_back({"particles", k_particleCount * k_particleStride});

        return description;
    }

    void record(vela::graphics::ComputeRecorder& recorder) override
    {
        vela::math::Mat4 params(0.0f);
        params[0][0] = m_deltaTime;
        params[0][1] = m_seeding ? 1.0f : 0.0f;

        recorder.bind(m_program);
        recorder.bindStorageBuffer(0, "particles");
        recorder.setConstants(params);
        recorder.dispatch((k_particleCount + k_workGroupSize - 1) / k_workGroupSize, 1, 1);

        m_seeding = false;
    }

private:
    vela::graphics::ComputeProgram& m_program;

    float m_deltaTime{0.0f};
    bool m_seeding{true};
};

class ParticleDrawPass : public vela::graphics::Pass
{
public:
    explicit ParticleDrawPass(vela::graphics::Material& material) : m_material(material) {}

    vela::graphics::PassDescription describe() const override
    {
        vela::graphics::AttachmentSlot colorSlot{};
        colorSlot.name = "color";
        colorSlot.format = vela::graphics::TextureFormat::RGBA16Float;
        colorSlot.load = vela::graphics::LoadOp::Clear;
        colorSlot.store = vela::graphics::StoreOp::Store;

        vela::graphics::PassDescription description{};
        description.colorOutputs.push_back(colorSlot);
        description.bufferInputs.push_back({"particles", vela::graphics::BufferAccess::Read});

        return description;
    }

    void record(vela::graphics::PassRecorder& recorder) override
    {
        recorder.bind(m_material);
        recorder.bindStorageBuffer(0, "particles");
        recorder.drawInstanced(6, k_particleCount);
    }

private:
    vela::graphics::Material& m_material;
};

int main()
{
    auto fail = [](const vela::Error& error)
    {
        std::cerr << "Vela error " << static_cast<uint32_t>(error.code) << ": " << error.message << '\n';
        return 1;
    };

    auto windowResult = vela::core::Window::create({.title = "Particles", .width = 900, .height = 900});

    if (!windowResult)
        return fail(windowResult.error());

    vela::core::Window window = std::move(windowResult).value();

    auto contextResult = vela::core::Context::create(window);

    if (!contextResult)
        return fail(contextResult.error());

    vela::core::Context ctx = std::move(contextResult).value();

    if (auto attached = ctx.attach(window); !attached)
        return fail(attached.error());

    auto computeResult = vela::assets::loadSpirv(
        vela::assets::resources::find("shaders/particles_update.comp.spv").string());
    auto vertexResult = vela::assets::loadSpirv(
        vela::assets::resources::find("shaders/particles_draw.vert.spv").string());
    auto fragmentResult = vela::assets::loadSpirv(
        vela::assets::resources::find("shaders/particles_draw.frag.spv").string());

    if (!computeResult) return fail(computeResult.error());
    if (!vertexResult) return fail(vertexResult.error());
    if (!fragmentResult) return fail(fragmentResult.error());

    const std::vector<uint32_t> computeShader = std::move(computeResult).value();
    const std::vector<uint32_t> vertexShader = std::move(vertexResult).value();
    const std::vector<uint32_t> fragmentShader = std::move(fragmentResult).value();

    auto programResult = vela::graphics::ComputeProgram::create(ctx, computeShader);

    if (!programResult)
        return fail(programResult.error());

    vela::graphics::ComputeProgram program = std::move(programResult).value();

    vela::graphics::RenderState renderState{};
    renderState.depthTest = false;
    renderState.depthWrite = false;
    renderState.cull = vela::graphics::CullMode::None;

    auto materialResult = vela::graphics::Material::create(ctx,
    {
        .vertexShader = vertexShader,
        .fragmentShader = fragmentShader,
        .vertexLayout = {},
        .renderState = renderState
    });

    if (!materialResult)
        return fail(materialResult.error());

    vela::graphics::Material material = std::move(materialResult).value();

    auto renderGraphResult = vela::graphics::RenderGraph::create(ctx);

    if (!renderGraphResult)
        return fail(renderGraphResult.error());

    vela::graphics::RenderGraph renderGraph = std::move(renderGraphResult).value();

    auto simulationPass = std::make_unique<SimulationPass>(program);
    SimulationPass& simulation = *simulationPass;

    if (auto added = renderGraph.addComputePass("simulate", std::move(simulationPass)); !added)
        return fail(added.error());

    if (auto added = renderGraph.addPass("particles", std::make_unique<ParticleDrawPass>(material)); !added)
        return fail(added.error());

    renderGraph.setPresentSource("color");

    auto lastTime = std::chrono::steady_clock::now();

    while (window.isOpen())
    {
        const auto currentTime = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();

        lastTime = currentTime;

        window.pollEvents();

        simulation.setDeltaTime(deltaTime);

        if (auto frame = renderGraph.execute(); !frame)
            return fail(frame.error());
    }

    ctx.waitIdle();

    return 0;
}
