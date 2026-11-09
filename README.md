# Vela

A Vulkan rendering library. You create a window, describe what you want drawn, and get a
frame. If the built-in path isn't what you need, you write your own render graph passes
and Vela schedules them - deriving execution order, barriers and layout transitions from
what each pass declares it touches.

Vela sits roughly where SFML sits - a library you build a renderer *with*, not a framework
you build a game *in* - but a layer lower: the unit of composition is a render graph pass,
not a sprite.

**Status: pre-1.0, API unstable.** It works, it is used, and names still move.

---

## Two ways to use it

**Use the built-ins.** Ask for a forward graph, fill a scene, execute. You never see a
pass, an attachment or a barrier.

**Write your own passes.** Implement `graphics::Pass`, declare your attachments and
inputs, add it to a graph. The built-ins are ordinary passes written against the same
public interface - they have no privileged access, which is the point.

Both use the same `RenderGraph`. You can mix them: keep the built-in scene pass and add
your own on top of its output.

---

## Quick start

```cpp
#include <Vela/Core/Window.hpp>
#include <Vela/Core/Context.hpp>
#include <Vela/Graphics/RenderScene.hpp>
#include <Vela/Builtins/ForwardGraph.hpp>
#include <Vela/Builtins/Shapes.hpp>
#include <Vela/Math/Math.hpp>

auto window = vela::core::Window::create({.title = "Cube", .width = 800, .height = 600});
if (!window) return 1;

auto context = vela::core::Context::create(window.value());
if (!context) return 1;

if (auto attached = context.value().attach(window.value()); !attached) return 1;

vela::graphics::RenderScene scene;

auto graph = vela::builtins::forwardGraph(context.value(), scene);
if (!graph) return 1;

const auto cubeData = vela::builtins::shapes3d::cube();
auto cube = vela::graphics::Mesh::create(context.value(), cubeData.vertices, cubeData.indices);

const vela::math::Mat4 view = vela::math::lookAt(
    vela::math::Vector3f(0.0f, 1.0f, 3.0f),
    vela::math::Vector3f(0.0f, 0.0f, 0.0f),
    vela::math::Vector3f(0.0f, 1.0f, 0.0f));

vela::math::Mat4 projection = vela::math::perspective(vela::math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
projection[1][1] *= -1.0f;

while (window.value().isOpen())
{
    window.value().pollEvents();

    scene.clear();
    scene.add(cube.value(), material, modelMatrix);

    graph.value().setView(view, projection);

    if (auto frame = graph.value().execute(); !frame)
        return 1;
}
```

`Examples/SimpleCube` is the complete version - materials, textures, window events and
frame stats. `Examples/LittleEngine` is a larger consumer: glTF loading, lighting, ImGui
and a fly camera.

## Errors

Nothing in the public API throws, and nothing fallible has a constructor. Anything that
can fail is a static `create()` returning `Result<T>`, or a method returning `Status`:

```cpp
auto mesh = vela::graphics::Mesh::create(context, vertices, indices);

if (!mesh)
{
    std::cerr << mesh.error().message << '\n';
    return 1;
}

vela::graphics::Mesh ready = std::move(mesh).value();
```

`Result<T>` is move-only and holds either a value or an `Error{code, message}`. `Status`
is the no-value form, for operations that either work or explain why not. Both are
`[[nodiscard]]` types: dropping one is a compiler warning.

---

## Writing a pass

A pass answers two questions: what does it touch, and what does it record.

```cpp
class OutlinePass : public vela::graphics::Pass
{
public:
    vela::graphics::PassDescription describe() const override
    {
        vela::graphics::AttachmentSlot color{};
        color.name = "outline";
        color.format = vela::graphics::TextureFormat::RGBA16Float;
        color.load = vela::graphics::LoadOp::Clear;
        color.store = vela::graphics::StoreOp::Store;

        vela::graphics::PassDescription description{};
        description.colorOutputs.push_back(color);
        description.inputs.push_back({"color", vela::graphics::InputUsage::Sampled});

        return description;
    }

    void record(vela::graphics::PassRecorder& recorder) override
    {
        recorder.draw(m_mesh, m_material, m_model);
    }
};

if (auto added = graph.addPass("outline", std::make_unique<OutlinePass>(...)); !added)
    return 1;

graph.setPresentSource("outline");
```

Attachments are matched **by name** across passes. A pass that lists `"color"` as an
input reads whatever an earlier pass wrote under that name, and that is the whole
dependency declaration - from it Vela derives execution order, image usage flags,
layout transitions and barriers, and culls passes whose output nothing reads.

You never write a barrier and never pick a layout. `AttachmentSlot::scale` sizes an
attachment relative to the swapchain, so a half-resolution pass is `scale = 0.5f`.

`setPresentSource` names the attachment that reaches the screen.

### Pass order

Order comes from declarations, never from guesswork:

- A pass runs after the producers of every attachment and buffer it lists as an input.
- A pass that writes an attachment with `LoadOp::Load` runs after every pass **added
  before it** that writes the same attachment. That is how overlays work: add the UI pass
  after the scene pass, load `"color"`, and it draws on top, whatever else the scene pass
  waits for.
- Passes the present source doesn't depend on are culled.

`removePass(name)` takes a pass out of the graph; `redescribePass(name)` re-reads a pass's
`describe()` (say, a new shadow-map resolution) without destroying it. Either way the
attachments are rebuilt on the next `execute()`, as after a window resize.

### MSAA

Give the attachments of a pass a sample count and name a resolve target:

```cpp
color.name = "colorMsaa";
color.samples = 4;              // power of two, up to DeviceInfo::maxMsaaSamples
color.resolveTarget = "color";  // single-sampled, resolved when the pass ends

depth.name = "depthMsaa";
depth.samples = 4;              // every output of a pass uses the same count
depth.resolveTarget = "depth";  // optional; depth resolves take sample zero
```

Later passes read or load `"color"` as usual; the resolve counts as an output of the
pass for ordering. A multisampled attachment can't be a pass input or the present source,
and `addPass` rejects a sample count the device can't render. Pipelines pick up the
pass's sample count on their own. The built-in path takes it as an option:

```cpp
vela::builtins::ScenePassOptions options{};
options.samples = std::min(4u, context.getDeviceInfo().maxMsaaSamples);
options.clear = {0.02f, 0.02f, 0.02f, 1.0f};

auto graph = vela::builtins::forwardGraph(context, scene, options);
```

The scene pass writes `k_sceneColorAttachment` (`"color"`, `RGBA16Float`) and
`k_sceneDepthAttachment` (`"depth"`, `Depth32Float`), with or without MSAA, so passes you
add after it don't change.

### Sampling attachments

`bindAttachment(slot, name)` samples with clamp-to-edge and linear filtering (nearest for
depth). Pass a `SamplerDescription` to choose: with a `compareOp`, a depth attachment is
read through a comparison sampler (`sampler2DShadow`), which with linear filtering gives
hardware 2x2 PCF. For shadow acne, set `RenderState::depthBiasConstant` and
`depthBiasSlope` on the material that renders the shadow map.

### Color

Attachments hold linear values; presenting to an sRGB swapchain encodes them. Colors that
arrive sRGB-encoded - vertex colors from an ImGui draw list, a picked UI color - must be
linearized in the shader, or they are encoded twice and look washed out.
`SwapchainInfo::srgb` says whether presenting encodes.

---

## Frames, timing and the window

`execute()` waits for the oldest frame in flight, acquires a swapchain image, records
every pass and presents. While the window is minimized it returns at once without
rendering; throttle your loop yourself if that matters.

- `getFrameStats()`: `cpuFrameMs` is the whole `execute()`, split into `cpuWaitMs` (fence
  and swapchain-image waits, where vsync usually blocks) and `cpuWorkMs` (recording,
  submitting, presenting). `gpuFrameMs` is GPU time of the newest finished frame.
- `getPassTimings()`: GPU time per pass, by name, in execution order.
- `readbackFrame(callback)`: captures the next presented frame as 8-bit sRGB RGBA and
  hands it to the callback a frame or two later, without stalling. For screenshots.
- `DynamicBuffer::update()` writes the copy the next recorded frame reads, so update
  before `execute()` (or inside a pass's `record()`), every frame you draw with it.
- Texture uploads never idle the GPU: `Texture::create()` and `Texture::update()` return
  once the pixels are in staging memory, and later frames are ordered after the copy.
  Loading hundreds of textures only waits when more than 256 MB of staging is in flight.
  `Texture::update()` replaces all pixels (same size and format), e.g. for a font atlas.
- `TextureOptions::generateMips` builds a texture's mip chain on the GPU from a single
  uncompressed level. BC formats need `Feature::TextureCompressionBC`, which Vela enables
  whenever the device has it; `SamplerDescription::anisotropy` needs
  `Feature::SamplerAnisotropy` requested and is clamped to the device limit.

### Present modes

`ContextPreferences::preferredVSync` picks the first mode; `Context::setVSync()` changes it
later and only the swapchain is recreated. Not every mode exists on every platform
(Wayland has no `VSync::Off`); a missing one falls back - `Off` to `Fast` to `On`, the
others to `On` - and `getSwapchainInfo().actualVSync` says what you got.
`getSupportedVSync()` lists what the surface offers. `VSync::LatestReady`
(`FIFO_LATEST_READY`) is used when the driver exposes it.

### Logging

Vela logs device selection, validation messages and failures to stdout/stderr by default.
`vela::setLogLevel(LogLevel::Warning)` drops the chatter; `vela::setLogCallback(...)`
routes everything into your own logger (`#include <Vela/Log.hpp>`).

### Input

`Window::events()` is the queue the last `pollEvents()` filled; the span is invalidated by
the next `pollEvents()` or `popEvent()`. `popEvent()` consumes from the front. For
mouse-look, `setCursorMode(CursorMode::Disabled)` plus `setRawMouseMotion(true)` reads
unaccelerated motion where the platform has it.

---

## Building

Requires **CMake 3.21+**, a **C++20** compiler, and the **Vulkan SDK** (or on Linux, the
Vulkan headers plus a `glslc` or `glslangValidator` for the example's shaders). Vela needs
a Vulkan 1.3 device - it uses dynamic rendering and synchronization2, and does not
implement fallbacks for either.

```sh
git clone --recursive https://github.com/Dlyvern/Vela.git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Debug builds define `VELA_DEBUG`, which enables the Vulkan validation layers. If you are
chasing a correctness bug and seeing no validation output, check that `CMAKE_BUILD_TYPE`
is actually set - an empty build type is not a debug build.

Linux also needs the X11 and Wayland development packages GLFW builds against
(`libxkbcommon-dev`, `libwayland-dev`, `wayland-protocols`, `libx11-dev`, `libxrandr-dev`,
`libxinerama-dev`, `libxcursor-dev`, `libxi-dev`). See
[.github/workflows/build.yml](.github/workflows/build.yml) for the exact list CI uses.

On Wayland, GLFW draws window decorations through libdecor, which loads GTK at runtime.
GTK keeps allocations alive until exit, so LeakSanitizer reports leaks from it at
shutdown that are not Vela's or yours. Suppress them (`leak:libgtk`, `leak:libdecor`), or
run under X11 (`XDG_SESSION_TYPE=x11`) when chasing leaks.

### Options

| Option | Default | Effect |
| --- | --- | --- |
| `VELA_WITH_GLFW` | `ON` | Build the GLFW windowing backend |
| `VELA_WITH_ASSETS` | `ON` | Build the asset loading target |
| `VELA_WITH_BUILTINS` | `ON` | Build the built-in passes, graph and primitives |
| `VELA_BUILD_EXAMPLES` | `ON` | Build `Examples/` |
| `VELA_INSTALL` | top-level | Generate install and export rules |

### Using Vela from your project

Installed:

```cmake
find_package(Vela REQUIRED)
target_link_libraries(app PRIVATE Vela::Vela Vela::Builtins Vela::Assets)
```

Or in-tree:

```cmake
add_subdirectory(vendor/Vela)
target_link_libraries(app PRIVATE Vela::Vela Vela::Builtins Vela::Assets)
```

Target names are identical either way, and the order you list them in doesn't matter.
Vela's dependencies — GLFW, volk, VMA, stb - are
Vela's problem: they are vendored, built statically, and exported as part of the package.
You need Vulkan itself, and nothing else.

---

## Current limitations

Real gaps, not aspirations:

- **Meshes are immutable.** There is no `Mesh::update`; geometry that changes goes in a
  `DynamicBuffer`. `Texture::update` replaces a whole texture, not a region.
- **No GPU mips for block-compressed textures.** Generate them before compressing.
- **MSAA depth resolves take sample zero.** Min/max depth resolve modes aren't exposed.
- **Pass timings cover the first 64 passes** of a frame.
- **Changing the graph rebuilds every attachment.** `addPass`, `removePass` and
  `redescribePass` re-create all attachments (with deferred destruction), not only the
  ones that changed.

---

## Scope

Vela is a **rendering library**, not a game engine. Entity systems, asset pipelines,
material registries, scene formats, camera controllers, editors and gameplay all belong
in the client.