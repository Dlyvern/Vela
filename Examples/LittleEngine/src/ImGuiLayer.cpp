#include "LittleEngine/ImGuiLayer.hpp"

#include "Vela/Graphics/RenderTypes.hpp"
#include "Vela/Math/Math.hpp"

#include <algorithm>
#include <iostream>

namespace little
{
    namespace
    {
        ImGuiKey toImGuiKey(vela::core::Key key)
        {
            using vela::core::Key;

            if (key >= Key::A && key <= Key::Z)
                return static_cast<ImGuiKey>(ImGuiKey_A + (static_cast<int>(key) - static_cast<int>(Key::A)));

            if (key >= Key::Num0 && key <= Key::Num9)
                return static_cast<ImGuiKey>(ImGuiKey_0 + (static_cast<int>(key) - static_cast<int>(Key::Num0)));

            if (key >= Key::F1 && key <= Key::F12)
                return static_cast<ImGuiKey>(ImGuiKey_F1 + (static_cast<int>(key) - static_cast<int>(Key::F1)));

            switch (key)
            {
                case Key::Escape:       return ImGuiKey_Escape;
                case Key::Enter:        return ImGuiKey_Enter;
                case Key::Tab:          return ImGuiKey_Tab;
                case Key::Backspace:    return ImGuiKey_Backspace;
                case Key::Space:        return ImGuiKey_Space;
                case Key::Delete:       return ImGuiKey_Delete;
                case Key::Left:         return ImGuiKey_LeftArrow;
                case Key::Right:        return ImGuiKey_RightArrow;
                case Key::Up:           return ImGuiKey_UpArrow;
                case Key::Down:         return ImGuiKey_DownArrow;
                case Key::Home:         return ImGuiKey_Home;
                case Key::End:          return ImGuiKey_End;
                case Key::LeftShift:    return ImGuiKey_LeftShift;
                case Key::LeftControl:  return ImGuiKey_LeftCtrl;
                case Key::LeftAlt:      return ImGuiKey_LeftAlt;
                case Key::RightShift:   return ImGuiKey_RightShift;
                case Key::RightControl: return ImGuiKey_RightCtrl;
                case Key::RightAlt:     return ImGuiKey_RightAlt;
                default:                return ImGuiKey_None;
            }
        }

        int toImGuiMouseButton(vela::core::MouseButton button)
        {
            switch (button)
            {
                case vela::core::MouseButton::Left:   return 0;
                case vela::core::MouseButton::Right:  return 1;
                case vela::core::MouseButton::Middle: return 2;
                default:                              return -1;
            }
        }
    } //namespace

    ImGuiPass::ImGuiPass(vela::core::Context& context, vela::graphics::Material& material,
        const std::string& colorAttachment)
        : m_material(material), m_colorAttachment(colorAttachment),
          m_vertexBuffer(vela::graphics::DynamicBuffer::create(context,
              vela::graphics::DynamicBuffer::Usage::Vertex, 0,
              vela::graphics::DynamicBuffer::IndexType::Uint16).value()),
          m_indexBuffer(vela::graphics::DynamicBuffer::create(context,
              vela::graphics::DynamicBuffer::Usage::Index, 0,
              vela::graphics::DynamicBuffer::IndexType::Uint16).value())
    {
    }

    vela::graphics::PassDescription ImGuiPass::describe() const
    {
        vela::graphics::AttachmentSlot colorSlot{};
        colorSlot.name = m_colorAttachment;
        colorSlot.format = vela::graphics::TextureFormat::RGBA16Float;
        colorSlot.load = vela::graphics::LoadOp::Load;
        colorSlot.store = vela::graphics::StoreOp::Store;

        vela::graphics::PassDescription description{};
        description.colorOutputs.push_back(colorSlot);

        return description;
    }

    void ImGuiPass::record(vela::graphics::PassRecorder& recorder)
    {
        ImDrawData* drawData = ImGui::GetDrawData();

        if (drawData == nullptr || drawData->CmdListsCount == 0)
            return;

        const vela::graphics::Extent2D extent = recorder.extent();

        m_vertices.clear();
        m_indices.clear();

        m_vertices.reserve(static_cast<size_t>(drawData->TotalVtxCount));
        m_indices.reserve(static_cast<size_t>(drawData->TotalIdxCount));

        for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex)
        {
            const ImDrawList* list = drawData->CmdLists[listIndex];

            m_vertices.insert(m_vertices.end(), list->VtxBuffer.Data, list->VtxBuffer.Data + list->VtxBuffer.Size);
            m_indices.insert(m_indices.end(), list->IdxBuffer.Data, list->IdxBuffer.Data + list->IdxBuffer.Size);
        }

        if (m_vertices.empty() || m_indices.empty())
            return;

        if (!m_vertexBuffer.update(std::as_bytes(std::span{m_vertices})))
            return;

        if (!m_indexBuffer.update(std::as_bytes(std::span{m_indices})))
            return;

        const vela::math::Mat4 projection = vela::math::ortho(
            drawData->DisplayPos.x,
            drawData->DisplayPos.x + drawData->DisplaySize.x,
            drawData->DisplayPos.y,
            drawData->DisplayPos.y + drawData->DisplaySize.y,
            -1.0f, 1.0f);

        recorder.bind(m_material);
        recorder.bindVertexBuffer(m_vertexBuffer);
        recorder.bindIndexBuffer(m_indexBuffer);
        recorder.setConstants(projection);

        uint32_t indexOffset = 0;
        uint32_t vertexOffset = 0;

        for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex)
        {
            const ImDrawList* list = drawData->CmdLists[listIndex];

            for (const ImDrawCmd& command : list->CmdBuffer)
            {
                if (command.UserCallback != nullptr)
                    continue;

                const int32_t left = std::max(static_cast<int32_t>(command.ClipRect.x), 0);
                const int32_t top = std::max(static_cast<int32_t>(command.ClipRect.y), 0);
                const int32_t right = std::min(static_cast<int32_t>(command.ClipRect.z),
                    static_cast<int32_t>(extent.width));
                const int32_t bottom = std::min(static_cast<int32_t>(command.ClipRect.w),
                    static_cast<int32_t>(extent.height));

                if (right <= left || bottom <= top)
                    continue;

                recorder.setScissor(left, top, static_cast<uint32_t>(right - left),
                    static_cast<uint32_t>(bottom - top));

                recorder.drawIndexed(command.ElemCount, indexOffset + command.IdxOffset,
                    vertexOffset + command.VtxOffset);
            }

            indexOffset += static_cast<uint32_t>(list->IdxBuffer.Size);
            vertexOffset += static_cast<uint32_t>(list->VtxBuffer.Size);
        }
    }

    vela::Status ImGuiLayer::initialise(vela::core::Context& context,
        std::span<const uint32_t> vertexShader, std::span<const uint32_t> fragmentShader)
    {
        m_context = &context;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        vela::graphics::VertexLayout layout
        {
            sizeof(ImDrawVert),
            {
                {0, vela::graphics::VertexAttributeFormat::Float2, offsetof(ImDrawVert, pos)},
                {1, vela::graphics::VertexAttributeFormat::Float2, offsetof(ImDrawVert, uv)},
                {2, vela::graphics::VertexAttributeFormat::Unorm8x4, offsetof(ImDrawVert, col)}
            }
        };

        vela::graphics::RenderState renderState{};
        renderState.blend = vela::graphics::BlendMode::Alpha;
        renderState.depthTest = false;
        renderState.depthWrite = false;
        renderState.cull = vela::graphics::CullMode::None;

        auto material = vela::graphics::Material::create(context,
        {
            .vertexShader = vertexShader,
            .fragmentShader = fragmentShader,
            .vertexLayout = layout,
            .renderState = renderState
        });

        if (!material)
            return material.error();

        m_material = std::make_unique<vela::graphics::Material>(std::move(material).value());

        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
        int channels = 0;

        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &channels);

        vela::graphics::ImageData fontImage{};
        fontImage.width = static_cast<uint32_t>(width);
        fontImage.height = static_cast<uint32_t>(height);
        fontImage.pixels = std::span<const std::byte>(reinterpret_cast<const std::byte*>(pixels),
            static_cast<size_t>(width) * height * channels);

        auto texture = vela::graphics::Texture::create(context, fontImage);

        if (!texture)
            return texture.error();

        m_fontTexture = std::make_unique<vela::graphics::Texture>(std::move(texture).value());

        if (auto bound = m_material->setTexture(0, *m_fontTexture); !bound)
            return bound.error();

        m_initialised = true;

        return {};
    }

    void ImGuiLayer::handleEvent(const vela::core::Event& event)
    {
        ImGuiIO& io = ImGui::GetIO();

        switch (event.type)
        {
            case vela::core::EventType::MouseMoved:
                io.AddMousePosEvent(event.mouseX, event.mouseY);
                break;

            case vela::core::EventType::MouseButtonPressed:
            case vela::core::EventType::MouseButtonReleased:
                if (const int button = toImGuiMouseButton(event.button); button >= 0)
                    io.AddMouseButtonEvent(button, event.type == vela::core::EventType::MouseButtonPressed);
                break;

            case vela::core::EventType::MouseScrolled:
                io.AddMouseWheelEvent(event.scrollX, event.scrollY);
                break;

            case vela::core::EventType::KeyPressed:
            case vela::core::EventType::KeyReleased:
                io.AddKeyEvent(ImGuiMod_Shift, event.modifiers.shift);
                io.AddKeyEvent(ImGuiMod_Ctrl, event.modifiers.control);
                io.AddKeyEvent(ImGuiMod_Alt, event.modifiers.alt);

                if (const ImGuiKey key = toImGuiKey(event.key); key != ImGuiKey_None)
                    io.AddKeyEvent(key, event.type == vela::core::EventType::KeyPressed);
                break;

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

    void ImGuiLayer::beginFrame(vela::core::Window& window, float deltaTime)
    {
        ImGuiIO& io = ImGui::GetIO();

        int width = 0;
        int height = 0;

        window.getFramebufferSize(width, height);

        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
        io.DeltaTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;

        ImGui::NewFrame();
    }

    std::unique_ptr<ImGuiPass> ImGuiLayer::createPass(const std::string& colorAttachment)
    {
        return std::make_unique<ImGuiPass>(*m_context, *m_material, colorAttachment);
    }

    bool ImGuiLayer::wantsMouse() const
    {
        return m_initialised && ImGui::GetIO().WantCaptureMouse;
    }

    bool ImGuiLayer::wantsKeyboard() const
    {
        return m_initialised && ImGui::GetIO().WantCaptureKeyboard;
    }

    ImGuiLayer::~ImGuiLayer()
    {
        if (m_initialised)
            ImGui::DestroyContext();
    }
} //namespace little
