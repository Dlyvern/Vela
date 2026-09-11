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
is the no-value form, for operations that either work or explain why not.

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

Target names are identical either way. Vela's dependencies — GLFW, volk, VMA, stb - are
Vela's problem: they are vendored, built statically, and exported as part of the package.
You need Vulkan itself, and nothing else.

---

## Current limitations

Real gaps, not aspirations:

- **Resources are immutable after creation.** There is no `Mesh::update` or
  `Texture::update`, so no particles, dynamic UI, debug lines or streaming yet.
- **Samplers are hardcoded** to linear filtering, repeat addressing and no mipmaps.
- **No MSAA.** Every attachment is single-sampled.
---

## Scope

Vela is a **rendering library**, not a game engine. Entity systems, asset pipelines,
material registries, scene formats, camera controllers, editors and gameplay all belong
in the client.