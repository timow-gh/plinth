#ifndef RENDERER_WINDOWSETTINGS_HPP
#define RENDERER_WINDOWSETTINGS_HPP

#include "plinth/plinth_export.h"
#include <cstdint>
#include <string>

namespace renderer {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251) // class 'std::xxx' needs to have dll-interface
#pragma warning(disable : 4275) // non dll-interface class 'std::xxx' used as base
#endif

struct PLINTH_EXPORT WindowSettings {
    const char* title = "plinth Viewer"; ///< Title of the window.
    std::uint32_t width = 1280;          ///< Width of the window.
    std::uint32_t height = 720;          ///< Height of the window.

    int red_bits = 8;                     ///< Number of bits for the red channel.
    int green_bits = 8;                   ///< Number of bits for the green channel.
    int blue_bits = 8;                    ///< Number of bits for the blue channel.
    int alpha_bits = 8;                   ///< Number of bits for the alpha channel.
    int depth_bits = 24;                  ///< Number of bits for the depth buffer.
    int stencil_bits = 8;                 ///< Number of bits for the stencil buffer.
    int accum_red_bits = 0;               ///< Number of bits for the red channel in the accumulation buffer.
    int accum_green_bits = 0;             ///< Number of bits for the green channel in the accumulation buffer.
    int accum_blue_bits = 0;              ///< Number of bits for the blue channel in the accumulation buffer.
    int accum_alpha_bits = 0;             ///< Number of bits for the alpha channel in the accumulation buffer.
    int aux_buffers = 0;                  ///< Number of auxiliary buffers.
    int samples = 8;                      ///< Number of samples for multisampling.
    int refresh_rate = -1;                ///< Refresh rate of the window, -1 for default.
    bool stereo = false;                  ///< Whether to use stereo rendering.
    bool srgb_capable = false;            ///< Whether the framebuffer should be sRGB capable.
    bool double_buffer = true;            ///< Whether to use double buffering.
    bool resizable = true;                ///< Whether the window is resizable.
    bool visible = true;                  ///< Whether the window is initially visible.
    bool decorated = true;                ///< Whether the window has decorations such as a border, a close widget, etc.
    bool focused = true;                  ///< Whether the window is initially focused.
    bool auto_iconify = true;             ///< Whether the window will be automatically iconified when switching
                                          ///< away from fullscreen.
    bool floating = false;                ///< Whether the window is always on top.
    bool maximized = false;               ///< Whether the window is maximized.
    bool center_cursor = false;           ///< Whether the cursor is centered over the window.
    bool transparent_framebuffer = false; ///< Whether the framebuffer should be transparent.
    bool focus_on_show = true;            ///< Whether the window should be focused when shown.
    bool scale_to_monitor = true; ///< Whether the window content should be scaled based on the monitor content scale.
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace renderer

#endif // RENDERER_WINDOWSETTINGS_HPP
