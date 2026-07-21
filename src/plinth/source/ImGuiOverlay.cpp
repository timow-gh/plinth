#include <algorithm>
#include <array>
#include <plinth/Assert.hpp>
#include <plinth/ImGuiOverlay.hpp>
#include <plinth/Warnings.hpp>
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

void ImGuiOverlay::add_post_processing_controls(
    float& exposureStops,
    renderer::ToneMapMode& toneMapMode,
    bool& fogEnabled,
    renderer::FogMode& fogMode,
    float& fogStart,
    float& fogEnd,
    float& fogDensity,
    std::array<float, 3>& fogColor,
    renderer::VisualizationMode& visualizationMode,
    float& hdrDisplayMax,
    bool& grayscale,
    bool& fxaaEnabled,
    float& fxaaEdgeThreshold,
    float& fxaaEdgeThresholdMin,
    float& fxaaSubpixelAmount) {

    float* fogColorPtr = fogColor.data();
    m_controls.emplace_back([&exposureStops, &toneMapMode,
                             &fogEnabled, &fogMode, &fogStart, &fogEnd, &fogDensity, fogColorPtr,
                             &visualizationMode, &hdrDisplayMax, &grayscale,
                             &fxaaEnabled, &fxaaEdgeThreshold, &fxaaEdgeThresholdMin, &fxaaSubpixelAmount]() {
        if (!ImGui::CollapsingHeader("Post Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }

        constexpr std::array<const char*, 10> visItems = {
            "Final", "Raw HDR", "Linear LDR (unencoded)", "Luminance",
            "Log Luminance", "Depth", "Overexposure",
            "Underexposure", "NaN & Infinity", "Grayscale"
        };
        int currentVis = static_cast<int>(visualizationMode);
        ImGui::Combo("Visualization", &currentVis, visItems.data(), static_cast<int>(visItems.size()));
        visualizationMode = static_cast<renderer::VisualizationMode>(currentVis);

        const bool exposureApplies = visualizationMode == renderer::VisualizationMode::Final ||
                                     visualizationMode == renderer::VisualizationMode::LinearLdr ||
                                     visualizationMode == renderer::VisualizationMode::Overexposure ||
                                     visualizationMode == renderer::VisualizationMode::Underexposure ||
                                     visualizationMode == renderer::VisualizationMode::Grayscale;
        if (exposureApplies) {
            ImGui::SliderFloat("Exposure (stops)", &exposureStops, -10.0F, 10.0F, "%.1F");  // NOLINT(readability-magic-numbers)
            ImGui::TextUnformatted("-1 = half, 0 = unchanged, +1 = twice");
        }

        const bool toneMappingApplies = visualizationMode == renderer::VisualizationMode::Final ||
                                        visualizationMode == renderer::VisualizationMode::LinearLdr ||
                                        visualizationMode == renderer::VisualizationMode::Grayscale;
        if (toneMappingApplies) {
            constexpr std::array<const char*, 2> toneMapItems = {"None (clamp)", "Reinhard"};
            int currentTM = static_cast<int>(toneMapMode);
            ImGui::Combo("Tone Mapping", &currentTM, toneMapItems.data(), static_cast<int>(toneMapItems.size()));
            toneMapMode = static_cast<renderer::ToneMapMode>(currentTM);
        }

        const bool fogApplies = visualizationMode != renderer::VisualizationMode::Depth &&
                                visualizationMode != renderer::VisualizationMode::NaNAndInfinity;
        if (fogApplies && ImGui::TreeNode("Fog")) {
            ImGui::Checkbox("Enabled", &fogEnabled);
            constexpr std::array<const char*, 2> fogItems = {"Linear", "Exponential"};
            int currentFog = static_cast<int>(fogMode);
            ImGui::Combo("Mode", &currentFog, fogItems.data(), static_cast<int>(fogItems.size()));
            fogMode = static_cast<renderer::FogMode>(currentFog);

            if (fogMode == renderer::FogMode::Linear) {
                constexpr float fogStartStep{0.1F};
                constexpr float fogEndMin{0.1F};
                constexpr float fogEndMax{500.0F};
                ImGui::SliderFloat("Start", &fogStart, 0.0F, fogEnd - fogStartStep);  // NOLINT(readability-magic-numbers)
                ImGui::SliderFloat("End", &fogEnd, fogStart + fogEndMin, fogEndMax);  // NOLINT(readability-magic-numbers)
            } else {
                constexpr float fogDensityMin{0.001F};
                constexpr float fogDensityMax{1.0F};
                ImGui::SliderFloat("Density", &fogDensity, fogDensityMin, fogDensityMax, "%.4F");
            }
            ImGui::ColorEdit3("Color", fogColorPtr);
            ImGui::TreePop();
        }

        if (visualizationMode == renderer::VisualizationMode::RawHdr ||
            visualizationMode == renderer::VisualizationMode::Luminance) {
            constexpr float hdrMin{0.1F};
            constexpr float hdrMax{100.0F};
            ImGui::SliderFloat("HDR Max", &hdrDisplayMax, hdrMin, hdrMax, "%.1F");
        }

        if (visualizationMode == renderer::VisualizationMode::Final) {
            ImGui::Checkbox("Grayscale", &grayscale);
        }

        ImGui::Separator();
        ImGui::Checkbox("FXAA", &fxaaEnabled);
        if (fxaaEnabled) {
            constexpr float edgeMin{0.0F};
            constexpr float edgeMax{0.5F};
            constexpr float edgeMinMin{0.0F};
            constexpr float edgeMinMax{0.25F};
            constexpr float subpixMin{0.0F};
            constexpr float subpixMax{1.0F};
            ImGui::SliderFloat("Edge Threshold", &fxaaEdgeThreshold, edgeMin, edgeMax, "%.3F");
            ImGui::SliderFloat("Minimum Edge Contrast", &fxaaEdgeThresholdMin, edgeMinMin, edgeMinMax, "%.4F");
            ImGui::SliderFloat("Subpixel Amount", &fxaaSubpixelAmount, subpixMin, subpixMax, "%.2F");
        }
    });
}

void ImGuiOverlay::build_controls() {
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
                for (const auto& control: m_controls) {
                    control();
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
    build_controls();
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

float ImGuiOverlay::get_reserved_control_panel_width() const {
    return m_controlPanelWidth + (controlPanelSideCount * controlPanelMargin);
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
