# plinth

`plinth` is a C++ OpenGL rendering library built on GLFW, glad, and ImGui.

## Renderer quick start

See [`examples/example_renderer_standalone.cpp`](examples/example_renderer_standalone.cpp) for a complete minimal renderer loop. Create one `renderer::Renderer`, add copied geometry or texture data, and run each frame in this order: `poll_events`, `begin_frame`, `draw`, and `end_frame`.

## Build and test

```sh
cmake --preset conf-gcc-debug
cmake --build --preset build-gcc-debug
ctest --preset test-gcc-debug --label-regex "unit|install"
xvfb-run --auto-servernum --server-args="-noreset -ac" ctest --preset test-gcc-debug --label-regex graphics
```

Linux graphics tests need an X server and Mesa OpenGL implementation. On headless systems, install Xvfb and Mesa, then use the `xvfb-run` command above; the test presets select software rendering with `LIBGL_ALWAYS_SOFTWARE=1`.
