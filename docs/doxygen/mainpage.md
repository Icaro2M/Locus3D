# Locus3D Production Documentation

This documentation describes the production code under `source/`.

The previous MVP code and vendored third-party tree have been removed from the production workspace. Production dependencies in `extern/` are excluded so the generated pages stay focused on Locus3D's own API.

## Current Focus

The most complete production subsystem today is `source/graphics/`. It contains the rendering foundation that higher-level editor, application, and modeling systems will use.

Useful entry points:

- `graphics/Graphics.h`: complete graphics API aggregator.
- `graphics/GraphicsCore.h`: core types, window/context, and configuration.
- `graphics/GraphicsGpu.h`: OpenGL resource wrappers and GPU state.
- `graphics/GraphicsRenderer.h`: scene rendering, cameras, materials, meshes, lighting, and viewport state.
- `graphics/GraphicsOverlay.h`: grid, axes, bounds, normals, measurements, and other viewport overlays.
- `graphics/GraphicsPicking.h`: picking buffers and object-id rendering.
- `graphics/GraphicsDebug.h`: debug drawing, logging, and GPU profiling.

## Intended Architecture

The graphics layer is designed to be consumed by higher-level systems rather than owning editor or application state directly.

- GPU wrappers manage OpenGL object lifetimes and binding concerns.
- Mesh upload/cache types translate CPU mesh data into GPU draw resources.
- Scene and renderer types organize renderable objects, materials, transforms, layers, visibility, and draw submission.
- Viewport and camera types hold presentation state needed for model inspection.
- Overlay and picking systems support editor interaction without becoming the editor state model.

Some systems that depend heavily on high-level interaction state, such as a future gizmo renderer, may remain intentionally thin or incomplete until those states exist.

## Regenerating

Run Doxygen from the repository root:

```powershell
doxygen Doxyfile
```

If CMake finds Doxygen during configuration, this is also available:

```powershell
cmake --build out/build/x64-debug --target docs
```

The generated HTML starts at `docs/generated/html/index.html`.
