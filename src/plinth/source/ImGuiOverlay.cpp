#include "plinth/ImGuiOverlay.hpp"

#include "plinth/Assert.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/Warnings.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <utility>

RENDERER_DISABLE_ALL_WARNINGS
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
RENDERER_ENABLE_ALL_WARNINGS

namespace renderer {

namespace {

constexpr float controlPanelMargin = 8.0F;
constexpr float controlPanelMinWidth = 280.0F;
constexpr float controlPanelMaxWidth = 480.0F;
constexpr float controlPanelTinyViewportMinWidth = 160.0F;
constexpr float resizeGripWidth = 8.0F;
constexpr float resizeGripLineInset = 2.0F;
constexpr float controlPanelSideCount = 2.0F;
constexpr float exposureStopsMin = -10.0F;
constexpr float exposureStopsMax = 10.0F;

float panel_max_width(float viewportWidth) {
    const float visibleWidth =
        std::max(controlPanelTinyViewportMinWidth, viewportWidth - (controlPanelSideCount * controlPanelMargin));
    return std::min(controlPanelMaxWidth, visibleWidth);
}

float panel_min_width(float viewportWidth) {
    return std::min(controlPanelMinWidth, panel_max_width(viewportWidth));
}

float clamp_panel_width(float width, float viewportWidth) {
    return std::clamp(width, panel_min_width(viewportWidth), panel_max_width(viewportWidth));
}

bool full_width_button(const char* label) {
    return ImGui::Button(label, ImVec2{-1.0F, 0.0F});
}

void render_panel_resize_grip(float& panelWidth, float viewportWidth, float height) {
    ImGui::InvisibleButton("##ControlPanelResizeGrip", ImVec2{resizeGripWidth, height});
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    if (hovered || active) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (active) {
        panelWidth = clamp_panel_width(panelWidth + ImGui::GetIO().MouseDelta.x, viewportWidth);
    }

    ImGuiCol gripColorIdx = ImGuiCol_Border;
    if (active) {
        gripColorIdx = ImGuiCol_ResizeGripActive;
    } else if (hovered) {
        gripColorIdx = ImGuiCol_ResizeGripHovered;
    }
    const ImU32 gripColor = ImGui::GetColorU32(gripColorIdx);
    const ImVec2 gripMin = ImGui::GetItemRectMin();
    const ImVec2 gripMax = ImGui::GetItemRectMax();
    const float gripCenterX = (gripMin.x + gripMax.x) * 0.5F;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddLine(ImVec2{gripCenterX, gripMin.y + resizeGripLineInset},
                      ImVec2{gripCenterX, gripMax.y - resizeGripLineInset},
                      gripColor,
                      1.0F);
}

} // namespace

ImGuiOverlay::ImGuiOverlay(void* nativeWindow)
    : m_window(nativeWindow) {
    RENDERER_ASSERT(m_window != nullptr);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* glfwWindow = static_cast<GLFWwindow*>(m_window);
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);
    ImGui_ImplOpenGL3_Init("#version 330");
}

ImGuiOverlay::~ImGuiOverlay() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiOverlay::new_frame() // NOLINT(readability-convert-member-functions-to-static)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiOverlay::add_control(std::function<void()> controlFunc) {
    m_controls.emplace_back(std::move(controlFunc));
}

void ImGuiOverlay::add_camera_controls(bool& autoZoomEnabled,
                                       CameraProjectionType& projectionType,
                                       bool& homeRequested) {
    m_controls.emplace_back([&autoZoomEnabled, &projectionType, &homeRequested]() {
        if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }

        ImGui::Checkbox("Auto Zoom", &autoZoomEnabled);

        constexpr std::array<const char*, 2> projectionItems = {"Perspective", "Orthographic"};
        int currentItem = static_cast<int>(projectionType);
        ImGui::TextUnformatted("Projection");
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::Combo("##Projection",
                         &currentItem,
                         projectionItems.data(),
                         static_cast<int>(projectionItems.size()))) {
            projectionType = static_cast<CameraProjectionType>(currentItem);
        }

        if (full_width_button("Home")) {
            homeRequested = true;
        }
    });
}

void ImGuiOverlay::add_lighting_controls(LightingConfig& lighting) {
    m_controls.emplace_back([&lighting]() {
        if (!ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }

        constexpr float positionMin{-100.0F};
        constexpr float positionMax{100.0F};
        constexpr float directionMin{-1.0F};
        constexpr float directionMax{1.0F};
        constexpr float shininessMin{1.0F};
        constexpr float shininessMax{256.0F};
        constexpr float attenuationMin{0.0F};
        constexpr float attenuationMax{2.0F};

        if (ImGui::TreeNodeEx("Key Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat3("Position", lighting.lightPosition.data(), positionMin, positionMax, "%.1F");
            ImGui::ColorEdit3("Color", lighting.lightColor.data());
            ImGui::SliderFloat3("Attenuation",
                                lighting.lightAttenuation.data(),
                                attenuationMin,
                                attenuationMax,
                                "%.3F");
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Fill Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat3("Direction", lighting.fillLightDir.data(), directionMin, directionMax, "%.2F");
            ImGui::ColorEdit3("Color", lighting.fillLightColor.data());
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Color", lighting.ambientColor.data());
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Ambient", lighting.materialAmbient.data());
            ImGui::ColorEdit3("Diffuse", lighting.materialDiffuse.data());
            ImGui::ColorEdit3("Specular", lighting.materialSpecular.data());
            ImGui::SliderFloat("Shininess", &lighting.shininess, shininessMin, shininessMax, "%.1F");
            ImGui::TreePop();
        }

        ImGui::Separator();
        // A default-constructed LightingConfig carries the member-initializer
        // defaults, so it is the single source of truth for "reset".
        if (full_width_button("Reset to defaults")) {
            lighting = LightingConfig{};
        }
    });
}

namespace {

void build_visualization_combo(Renderer& renderer) {
    constexpr std::array<const char*, 10> visItems = {"Final",
                                                      "Raw HDR",
                                                      "Linear LDR (unencoded)",
                                                      "Luminance",
                                                      "Log Luminance",
                                                      "Depth",
                                                      "Overexposure",
                                                      "Underexposure",
                                                      "NaN & Infinity",
                                                      "Grayscale"};
    int currentVis = static_cast<int>(renderer.get_visualization_mode());
    if (ImGui::Combo("Visualization", &currentVis, visItems.data(), static_cast<int>(visItems.size()))) {
        renderer.set_visualization_mode(static_cast<renderer::VisualizationMode>(currentVis));
    }
}

void build_exposure_control(Renderer& renderer, renderer::VisualizationMode mode) {
    const bool exposureApplies =
        mode == renderer::VisualizationMode::Final || mode == renderer::VisualizationMode::LinearLdr ||
        mode == renderer::VisualizationMode::Overexposure || mode == renderer::VisualizationMode::Underexposure ||
        mode == renderer::VisualizationMode::Grayscale;
    if (!exposureApplies) {
        return;
    }
    float exposureStops = renderer.get_exposure_stops();
    if (ImGui::SliderFloat("Exposure (stops)", &exposureStops, exposureStopsMin, exposureStopsMax, "%.1F")) {
        renderer.set_exposure_stops(exposureStops);
    }
    ImGui::TextUnformatted("-1 = half, 0 = unchanged, +1 = twice");
}

void build_tone_mapping_control(Renderer& renderer, renderer::VisualizationMode mode) {
    const bool toneMappingApplies = mode == renderer::VisualizationMode::Final ||
                                    mode == renderer::VisualizationMode::LinearLdr ||
                                    mode == renderer::VisualizationMode::Grayscale;
    if (!toneMappingApplies) {
        return;
    }
    constexpr std::array<const char*, 2> toneMapItems = {"None (clamp)", "Reinhard"};
    int currentTM = static_cast<int>(renderer.get_tone_map_mode());
    if (ImGui::Combo("Tone Mapping", &currentTM, toneMapItems.data(), static_cast<int>(toneMapItems.size()))) {
        renderer.set_tone_map_mode(static_cast<renderer::ToneMapMode>(currentTM));
    }
}

void build_hdr_display_control(Renderer& renderer, renderer::VisualizationMode mode) {
    if (mode != renderer::VisualizationMode::RawHdr && mode != renderer::VisualizationMode::Luminance) {
        return;
    }
    constexpr float hdrMin{0.1F};
    constexpr float hdrMax{100.0F};
    float hdrDisplayMax = renderer.get_hdr_display_max();
    if (ImGui::SliderFloat("HDR Max", &hdrDisplayMax, hdrMin, hdrMax, "%.1F")) {
        renderer.set_hdr_display_max(hdrDisplayMax);
    }
}

void build_grayscale_control(Renderer& renderer, renderer::VisualizationMode mode) {
    if (mode != renderer::VisualizationMode::Final) {
        return;
    }
    bool grayscale = renderer.get_grayscale();
    if (ImGui::Checkbox("Grayscale", &grayscale)) {
        renderer.set_grayscale(grayscale);
    }
}

void build_fxaa_control(Renderer& renderer) {
    bool fxaaEnabled = renderer.get_fxaa_enabled();
    if (ImGui::Checkbox("FXAA", &fxaaEnabled)) {
        renderer.set_fxaa_enabled(fxaaEnabled);
    }
    if (!renderer.get_fxaa_enabled()) {
        return;
    }
    constexpr float edgeMin{0.0F};
    constexpr float edgeMax{0.5F};
    constexpr float edgeMinMin{0.0F};
    constexpr float edgeMinMax{0.25F};
    constexpr float subpixMin{0.0F};
    constexpr float subpixMax{1.0F};
    float edgeThreshold = renderer.get_fxaa_edge_threshold();
    if (ImGui::SliderFloat("Edge Threshold", &edgeThreshold, edgeMin, edgeMax, "%.3F")) {
        renderer.set_fxaa_edge_threshold(edgeThreshold);
    }
    float edgeThresholdMin = renderer.get_fxaa_edge_threshold_min();
    if (ImGui::SliderFloat("Minimum Edge Contrast", &edgeThresholdMin, edgeMinMin, edgeMinMax, "%.4F")) {
        renderer.set_fxaa_edge_threshold_min(edgeThresholdMin);
    }
    float subpixelAmount = renderer.get_fxaa_subpixel_amount();
    if (ImGui::SliderFloat("Subpixel Amount", &subpixelAmount, subpixMin, subpixMax, "%.2F")) {
        renderer.set_fxaa_subpixel_amount(subpixelAmount);
    }
}

void build_msaa_control(Renderer& renderer) {
    const int currentSamples = renderer.get_msaa_samples();
    const int maxSamples = renderer.get_max_msaa_samples();
    const std::string preview = currentSamples == 1 ? "Off (1x)" : std::format("{}x", currentSamples);

    if (!ImGui::BeginCombo("MSAA", preview.c_str())) {
        return;
    }

    const auto addSampleChoice = [&renderer, currentSamples](int samples, const std::string& label) {
        const bool selected = samples == currentSamples;
        if (ImGui::Selectable(label.c_str(), selected)) {
            renderer.set_msaa_samples(samples);
        }
        if (selected) {
            ImGui::SetItemDefaultFocus();
        }
    };

    addSampleChoice(1, "Off (1x)");
    for (int samples = 2; samples <= maxSamples;) {
        addSampleChoice(samples, std::format("{}x", samples));
        if (samples > maxSamples / 2) {
            break;
        }
        samples *= 2;
    }
    ImGui::EndCombo();
}

} // namespace

void ImGuiOverlay::add_post_processing_controls(Renderer& renderer) {
    m_controls.emplace_back([&renderer]() {
        if (!ImGui::CollapsingHeader("Post Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }
        build_visualization_combo(renderer);
        const renderer::VisualizationMode vizMode = renderer.get_visualization_mode();
        build_exposure_control(renderer, vizMode);
        build_tone_mapping_control(renderer, vizMode);
        build_hdr_display_control(renderer, vizMode);
        build_grayscale_control(renderer, vizMode);
        ImGui::Separator();
        build_msaa_control(renderer);
        build_fxaa_control(renderer);
    });
}

void ImGuiOverlay::add_release_post_processing_controls(Renderer& renderer) {
    m_controls.emplace_back([&renderer]() {
        if (!ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }

        build_msaa_control(renderer);
        bool fxaaEnabled = renderer.get_fxaa_enabled();
        if (ImGui::Checkbox("FXAA", &fxaaEnabled)) {
            renderer.set_fxaa_enabled(fxaaEnabled);
        }

        float exposureStops = renderer.get_exposure_stops();
        if (ImGui::SliderFloat("Exposure (stops)", &exposureStops, exposureStopsMin, exposureStopsMax, "%.1F")) {
            renderer.set_exposure_stops(exposureStops);
        }

    });
}

void ImGuiOverlay::build_controls(OverlayFrameContext& ctx) {
    // Report the window region left free for the 3D scene: the main viewport minus the
    // left control panel (its margins on both sides plus the current panel width). The
    // Renderer applies this as the scene viewport when the app hasn't set one. The panel
    // width comes from the previous frame's layout, so a resize takes effect one frame later.
    if (const ImGuiViewport* viewport = ImGui::GetMainViewport(); viewport != nullptr) {
        const float panelWidth = clamp_panel_width(m_controlPanelWidth, viewport->WorkSize.x);
        const float reservedLeft = (controlPanelSideCount * controlPanelMargin) + panelWidth;
        const float sceneWidth = std::max(1.0F, viewport->WorkSize.x - reservedLeft);
        ctx.sceneViewportHint = LogicalViewportRect{static_cast<double>(viewport->WorkPos.x + reservedLeft),
                                                    static_cast<double>(viewport->WorkPos.y),
                                                    static_cast<double>(sceneWidth),
                                                    static_cast<double>(viewport->WorkSize.y)};
    }

    add_camera_controls(ctx.autoFitEnabled, ctx.projectionType, ctx.homeRequested);
    if (m_uiMode == UiMode::Debug) {
        add_post_processing_controls(ctx.renderer);
    } else {
        // The game-like Release panel exposes no debug visualizations, so pin those back to
        // sensible defaults; leftover debug state (e.g. a Depth view) must not persist here.
        ctx.renderer.set_visualization_mode(VisualizationMode::Final);
        ctx.renderer.set_grayscale(false);
        add_release_post_processing_controls(ctx.renderer);
    }
}

void ImGuiOverlay::layout_controls() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport != nullptr) {
        m_controlPanelWidth = clamp_panel_width(m_controlPanelWidth, viewport->WorkSize.x);

        ImGui::SetNextWindowPos(
            ImVec2{viewport->WorkPos.x + controlPanelMargin, viewport->WorkPos.y + controlPanelMargin},
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2{m_controlPanelWidth,
                   std::max(0.0F, viewport->WorkSize.y - (controlPanelSideCount * controlPanelMargin))},
            ImGuiCond_Always);

        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("Controls", nullptr, windowFlags)) {
            const ImVec2 availableContentSize = ImGui::GetContentRegionAvail();
            const float contentWidth = std::max(1.0F, availableContentSize.x - resizeGripWidth);
            if (ImGui::BeginChild("##ControlPanelContent", ImVec2{contentWidth, 0.0F}, 0)) {
                int id = 0;
                for (const auto& control: m_controls) {
                    ImGui::PushID(id++);
                    control();
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();

            ImGui::SameLine(0.0F, 0.0F);
            render_panel_resize_grip(m_controlPanelWidth, viewport->WorkSize.x, availableContentSize.y);
        }
        ImGui::End();
    }
    m_controls.clear();
}

void ImGuiOverlay::render() {
    layout_controls();
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData != nullptr && drawData->Valid) {
        ImGui_ImplOpenGL3_RenderDrawData(drawData);
    }
}

void ImGuiOverlay::end_frame() // NOLINT(readability-convert-member-functions-to-static)
{
    ImGui::EndFrame();
}

bool ImGuiOverlay::wants_mouse() const // NOLINT(readability-convert-member-functions-to-static)
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiOverlay::wants_keyboard() const // NOLINT(readability-convert-member-functions-to-static)
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiOverlay::handle_cursor_position(double xpos, double ypos) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* glfwWindow = static_cast<GLFWwindow*>(m_window);
    ImGui_ImplGlfw_CursorPosCallback(glfwWindow, xpos, ypos);
    return m_inputCaptureState.should_forward_cursor_position(ImGui::GetIO().WantCaptureMouse);
}

bool ImGuiOverlay::handle_mouse_button(int button, Action action, Mods mods) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* glfwWindow = static_cast<GLFWwindow*>(m_window);
    ImGui_ImplGlfw_MouseButtonCallback(glfwWindow, button, static_cast<int>(action), static_cast<int>(mods));
    return m_inputCaptureState.should_forward_mouse_button(button, action, ImGui::GetIO().WantCaptureMouse);
}

bool ImGuiOverlay::handle_scroll(double xoffset, double yoffset) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* glfwWindow = static_cast<GLFWwindow*>(m_window);
    ImGui_ImplGlfw_ScrollCallback(glfwWindow, xoffset, yoffset);
    return m_inputCaptureState.should_forward_scroll(ImGui::GetIO().WantCaptureMouse);
}

bool ImGuiOverlay::handle_key(Key key, Scancode scancode, Action action, Mods mods) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* glfwWindow = static_cast<GLFWwindow*>(m_window);
    ImGui_ImplGlfw_KeyCallback(glfwWindow,
                               static_cast<int>(key),
                               scancode.get_value(),
                               static_cast<int>(action),
                               static_cast<int>(mods));
    return InputCaptureState::should_forward_key(ImGui::GetIO().WantCaptureKeyboard);
}

void ImGuiOverlay::handle_char(std::uint32_t codepoint) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* glfwWindow = static_cast<GLFWwindow*>(m_window);
    ImGui_ImplGlfw_CharCallback(glfwWindow, codepoint);
}

} // namespace renderer
