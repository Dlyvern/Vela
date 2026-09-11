#include <algorithm>
#include <iostream>
#include "imgui.h"

#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/DynamicBuffer.hpp"
#include "Vela/Graphics/RenderGraph.hpp"
#include "Vela/Core/Context.hpp"
#include "Vela/Core/Window.hpp"
#include "Vela/Assets/Resources.hpp"
#include "Vela/Assets/Shader.hpp"
#include "Vela/Math/Math.hpp"
#include "Vela/Graphics/Texture.hpp"

ImGuiKey toImGuiKey(vela::core::Key key)
{
    using vela::core::Key;

    if (key >= Key::A && key <= Key::Z)
        return static_cast<ImGuiKey>(ImGuiKey_A + (static_cast<int>(key) - static_cast<int>(Key::A)));

    if (key >= Key::Num0 && key <= Key::Num9)
        return static_cast<ImGuiKey>(ImGuiKey_0 + (static_cast<int>(key) - static_cast<int>(Key::Num0)));

    if (key >= Key::Numpad0 && key <= Key::Numpad9)
        return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + (static_cast<int>(key) - static_cast<int>(Key::Numpad0)));

    if (key >= Key::F1 && key <= Key::F12)
        return static_cast<ImGuiKey>(ImGuiKey_F1 + (static_cast<int>(key) - static_cast<int>(Key::F1)));

    switch (key)
    {
        case Key::Escape:        return ImGuiKey_Escape;
        case Key::Enter:         return ImGuiKey_Enter;
        case Key::Tab:           return ImGuiKey_Tab;
        case Key::Backspace:     return ImGuiKey_Backspace;
        case Key::Space:         return ImGuiKey_Space;
        case Key::Insert:        return ImGuiKey_Insert;
        case Key::Delete:        return ImGuiKey_Delete;

        case Key::Left:          return ImGuiKey_LeftArrow;
        case Key::Right:         return ImGuiKey_RightArrow;
        case Key::Up:            return ImGuiKey_UpArrow;
        case Key::Down:          return ImGuiKey_DownArrow;

        case Key::PageUp:        return ImGuiKey_PageUp;
        case Key::PageDown:      return ImGuiKey_PageDown;
        case Key::Home:          return ImGuiKey_Home;
        case Key::End:           return ImGuiKey_End;

        case Key::CapsLock:      return ImGuiKey_CapsLock;
        case Key::ScrollLock:    return ImGuiKey_ScrollLock;
        case Key::NumLock:       return ImGuiKey_NumLock;
        case Key::PrintScreen:   return ImGuiKey_PrintScreen;
        case Key::Pause:         return ImGuiKey_Pause;

        case Key::LeftShift:     return ImGuiKey_LeftShift;
        case Key::LeftControl:   return ImGuiKey_LeftCtrl;
        case Key::LeftAlt:       return ImGuiKey_LeftAlt;
        case Key::LeftSuper:     return ImGuiKey_LeftSuper;
        case Key::RightShift:    return ImGuiKey_RightShift;
        case Key::RightControl:  return ImGuiKey_RightCtrl;
        case Key::RightAlt:      return ImGuiKey_RightAlt;
        case Key::RightSuper:    return ImGuiKey_RightSuper;
        case Key::Menu:          return ImGuiKey_Menu;

        case Key::Apostrophe:    return ImGuiKey_Apostrophe;
        case Key::Comma:         return ImGuiKey_Comma;
        case Key::Minus:         return ImGuiKey_Minus;
        case Key::Period:        return ImGuiKey_Period;
        case Key::Slash:         return ImGuiKey_Slash;
        case Key::Semicolon:     return ImGuiKey_Semicolon;
        case Key::Equal:         return ImGuiKey_Equal;
        case Key::LeftBracket:   return ImGuiKey_LeftBracket;
        case Key::Backslash:     return ImGuiKey_Backslash;
        case Key::RightBracket:  return ImGuiKey_RightBracket;
        case Key::GraveAccent:   return ImGuiKey_GraveAccent;

        case Key::NumpadDecimal:  return ImGuiKey_KeypadDecimal;
        case Key::NumpadDivide:   return ImGuiKey_KeypadDivide;
        case Key::NumpadMultiply: return ImGuiKey_KeypadMultiply;
        case Key::NumpadSubtract: return ImGuiKey_KeypadSubtract;
        case Key::NumpadAdd:      return ImGuiKey_KeypadAdd;
        case Key::NumpadEnter:    return ImGuiKey_KeypadEnter;
        case Key::NumpadEqual:    return ImGuiKey_KeypadEqual;

        default:                 return ImGuiKey_None;
    }
}

int toImGuiMouseButton(vela::core::MouseButton button)
{
    switch (button)
    {
        case vela::core::MouseButton::Left:   return 0;
        case vela::core::MouseButton::Right:  return 1;
        case vela::core::MouseButton::Middle: return 2;
        case vela::core::MouseButton::Extra1: return 3;
        case vela::core::MouseButton::Extra2: return 4;
        default:                              return -1;
    }
}

class ImguiPass : public vela::graphics::Pass
{
public:
    ImguiPass(vela::core::Context& context, vela::graphics::Material& material) : m_context(context), m_material(material),
    m_vertexBuffer(vela::graphics::DynamicBuffer::create(context, vela::graphics::DynamicBuffer::Usage::Vertex, 0, 
        vela::graphics::DynamicBuffer::IndexType::Uint16).value()),
    m_indexBuffer(vela::graphics::DynamicBuffer::create(context, vela::graphics::DynamicBuffer::Usage::Index, 0,
    vela::graphics::DynamicBuffer::IndexType::Uint16).value())
    {
        
    }

    vela::graphics::PassDescription describe() const override
    {
        vela::graphics::AttachmentSlot colorSlot{};
        colorSlot.format = vela::graphics::TextureFormat::RGBA16Float;
        colorSlot.load = vela::graphics::LoadOp::Clear;
        colorSlot.store = vela::graphics::StoreOp::Store;
        colorSlot.name = "color";

        vela::graphics::PassDescription passDescription{};
        passDescription.colorOutputs.push_back(colorSlot);

        return passDescription;
    }

    void record(vela::graphics::PassRecorder& recorder) override
    {
        ImDrawData* imguiData = ImGui::GetDrawData();

        if (imguiData == nullptr || imguiData->CmdListsCount == 0)
            return;

        const vela::graphics::Extent2D extent = recorder.extent();

        const vela::math::Mat4 projection = vela::math::ortho(
            imguiData->DisplayPos.x,
            imguiData->DisplayPos.x + imguiData->DisplaySize.x,
            imguiData->DisplayPos.y,
            imguiData->DisplayPos.y + imguiData->DisplaySize.y,
            -1.0f,
            1.0f
        );

        m_vertices.clear();
        m_indices.clear();

        m_vertices.reserve(static_cast<size_t>(imguiData->TotalVtxCount));
        m_indices.reserve(static_cast<size_t>(imguiData->TotalIdxCount));

        for (int i = 0; i < imguiData->CmdListsCount; ++i)
        {
            const ImDrawList* list = imguiData->CmdLists[i];

            m_vertices.insert(m_vertices.end(), list->VtxBuffer.Data,
                list->VtxBuffer.Data + list->VtxBuffer.Size);

            m_indices.insert(m_indices.end(), list->IdxBuffer.Data,
                list->IdxBuffer.Data + list->IdxBuffer.Size);
        }

        if (m_vertices.empty() || m_indices.empty())
            return;

        if (auto uploaded = m_vertexBuffer.update(std::as_bytes(std::span{m_vertices})); !uploaded)
            return;

        if (auto uploaded = m_indexBuffer.update(std::as_bytes(std::span{m_indices})); !uploaded)
            return;

        recorder.bind(m_material);

        recorder.bindVertexBuffer(m_vertexBuffer);
        recorder.bindIndexBuffer(m_indexBuffer);

        recorder.setConstants(projection);

        uint32_t indexOffset{0};
        uint32_t vertexOffset{0};

        for (int i = 0; i < imguiData->CmdListsCount; ++i)
        {
            const ImDrawList* list = imguiData->CmdLists[i];

            for (const ImDrawCmd& imguiCmd : list->CmdBuffer)
            {
                if (imguiCmd.UserCallback != nullptr)
                    continue;

                const ImVec4 clip = imguiCmd.ClipRect;

                const int32_t left = std::max(static_cast<int32_t>(clip.x), 0);
                const int32_t top = std::max(static_cast<int32_t>(clip.y), 0);
                const int32_t right = std::min(static_cast<int32_t>(clip.z), static_cast<int32_t>(extent.width));
                const int32_t bottom = std::min(static_cast<int32_t>(clip.w), static_cast<int32_t>(extent.height));

                if (right <= left || bottom <= top)
                    continue;

                recorder.setScissor(left, top, static_cast<uint32_t>(right - left),
                    static_cast<uint32_t>(bottom - top));

                recorder.drawIndexed(imguiCmd.ElemCount, indexOffset + imguiCmd.IdxOffset, 
                    vertexOffset + imguiCmd.VtxOffset);
            }

            indexOffset += list->IdxBuffer.Size; 
            vertexOffset += list->VtxBuffer.Size;
        }
    }
private:
    vela::graphics::DynamicBuffer m_vertexBuffer;
    vela::graphics::DynamicBuffer m_indexBuffer;

    std::vector<ImDrawVert> m_vertices;
    std::vector<ImDrawIdx> m_indices;

    vela::core::Context& m_context;
    vela::graphics::Material& m_material;
};

int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();
    

    auto fail = [](const vela::Error& error)
    {
        std::cerr << "Vela error " << static_cast<uint32_t>(error.code) << ": " << error.message << '\n';
        return 1;
    };

    auto windowResult = vela::core::Window::create({.title = "ImguiIntegration", .width = 800, .height = 600});

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

    auto staticVertResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/imgui_shader.vert.spv").string());
    auto staticFragResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/imgui_shader.frag.spv").string());

    if (!staticVertResult) return fail(staticVertResult.error());
    if (!staticFragResult) return fail(staticFragResult.error());

    const std::vector<uint32_t> staticVert = std::move(staticVertResult).value();
    const std::vector<uint32_t> staticFrag = std::move(staticFragResult).value();

    vela::graphics::RenderState renderState{};
    renderState.blend = vela::graphics::BlendMode::Alpha;
    renderState.depthTest = false;
    renderState.cull = vela::graphics::CullMode::None;

    vela::graphics::VertexLayout imguiVertexLayout
    {
        sizeof(ImDrawVert),
        {
            {0, vela::graphics::VertexAttributeFormat::Float2, offsetof(ImDrawVert, pos)},
            {1, vela::graphics::VertexAttributeFormat::Float2, offsetof(ImDrawVert, uv)},
            {2, vela::graphics::VertexAttributeFormat::Unorm8x4, offsetof(ImDrawVert, col)}
        }
    };

    auto materialResult = vela::graphics::Material::create(ctx,
    {
        .vertexShader = staticVert,
        .fragmentShader = staticFrag,
        .vertexLayout = imguiVertexLayout,
        .renderState = renderState
    });

    if (!materialResult)
        return fail(materialResult.error());

    vela::graphics::Material material = std::move(materialResult).value();

    auto renderGraphResult = vela::graphics::RenderGraph::create(ctx);

    if (!renderGraphResult)
        return fail(renderGraphResult.error());

    vela::graphics::RenderGraph renderGraph = std::move(renderGraphResult).value();

    if (auto added = renderGraph.addPass("imgui", std::make_unique<ImguiPass>(ctx, material)); !added)
        return fail(added.error());

    renderGraph.setPresentSource("color");

    ImGuiIO& io = ImGui::GetIO();

    vela::graphics::ImageData imageData;

    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;

    io.Fonts->GetTexDataAsRGBA32(
        &pixels,
        &width,
        &height,
        &channels
    );

    std::span<const std::byte> data{reinterpret_cast<const std::byte*>(pixels),
    static_cast<size_t>(width) * height * channels};

    imageData.height = height;
    imageData.width = width;
    imageData.pixels = data;

    auto textureResult = vela::graphics::Texture::create(ctx, imageData);

    if(!textureResult)
        return fail(textureResult.error());

    vela::graphics::Texture texture = std::move(textureResult).value();

    material.setTexture(0, texture);

    static auto lastTime = std::chrono::steady_clock::now();

    vela::core::Event event{};

    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    while(window.isOpen())
    {
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();

        lastTime = currentTime;

        window.pollEvents();

        while(window.popEvent(event))
        {
            switch (event.type)
            {
                case vela::core::EventType::MouseMoved:
                    io.AddMousePosEvent(event.mouseX, event.mouseY);
                    break;

                case vela::core::EventType::MouseButtonPressed:
                case vela::core::EventType::MouseButtonReleased:
                    if (const int button = toImGuiMouseButton(event.button); button >= 0)
                        io.AddMouseButtonEvent(button,
                            event.type == vela::core::EventType::MouseButtonPressed);
                    break;

                case vela::core::EventType::MouseScrolled:
                    io.AddMouseWheelEvent(event.scrollX, event.scrollY);
                    break;

                case vela::core::EventType::KeyPressed:
                case vela::core::EventType::KeyReleased:
                {
                    io.AddKeyEvent(ImGuiMod_Shift, event.modifiers.shift);
                    io.AddKeyEvent(ImGuiMod_Ctrl, event.modifiers.control);
                    io.AddKeyEvent(ImGuiMod_Alt, event.modifiers.alt);

                    if (const ImGuiKey key = toImGuiKey(event.key); key != ImGuiKey_None)
                        io.AddKeyEvent(key, event.type == vela::core::EventType::KeyPressed);

                    break;
                }

                case vela::core::EventType::TextEntered:
                    io.AddInputCharacter(event.codepoint);
                    break;

                case vela::core::EventType::FocusGained:
                    io.AddFocusEvent(true);
                    break;

                case vela::core::EventType::FocusLost:
                    io.AddFocusEvent(false);
                    break;

                default:
                    break;
            }
        }

        int width, height;

        window.getFramebufferSize(width, height);

        io.DisplaySize = ImVec2(
            static_cast<float>(width),
            static_cast<float>(height)
        );

        io.DeltaTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;

        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        if (auto frame = renderGraph.execute(); !frame)
            return fail(frame.error());
    }

    ImGui::DestroyContext();

    ctx.waitIdle();
}
