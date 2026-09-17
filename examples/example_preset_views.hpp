#pragma once

#include "plinth/Renderer.hpp"

namespace example {

/// Registers keys 1-7 as preset-view shortcuts (front/back/left/right/top/bottom/iso).
/// FRONT..BOTTOM keep the FIX_ROTATE view mode; ISO switches to NONE so it reads as a free
/// orbit start. Shared by the example programs so the mapping lives in one place.
inline renderer::CallbackSubscription add_preset_view_callback(renderer::Renderer& renderer) {
    return renderer.add_key_callback([&renderer](renderer::Key key,
                                                 renderer::Scancode /*scancode*/,
                                                 renderer::Action action,
                                                 renderer::Mods /*mods*/) {
        if (action != renderer::Action::PRESS) {
            return;
        }
        renderer::CameraInteractor::CameraViewMode viewMode = renderer::CameraInteractor::CameraViewMode::FIX_ROTATE;
        switch (key) {
        case renderer::Key::KEY_1: renderer.go_to_preset_view(renderer::PresetView::FRONT); break;
        case renderer::Key::KEY_2: renderer.go_to_preset_view(renderer::PresetView::BACK); break;
        case renderer::Key::KEY_3: renderer.go_to_preset_view(renderer::PresetView::LEFT); break;
        case renderer::Key::KEY_4: renderer.go_to_preset_view(renderer::PresetView::RIGHT); break;
        case renderer::Key::KEY_5: renderer.go_to_preset_view(renderer::PresetView::TOP); break;
        case renderer::Key::KEY_6: renderer.go_to_preset_view(renderer::PresetView::BOTTOM); break;
        case renderer::Key::KEY_7:
            renderer.go_to_preset_view(renderer::PresetView::ISO);
            viewMode = renderer::CameraInteractor::CameraViewMode::NONE;
            break;
        default: return;
        }
        auto camera = renderer.get_camera().lock();
        if (camera) {
            camera->set_view_mode(viewMode);
        }
    });
}

} // namespace example
