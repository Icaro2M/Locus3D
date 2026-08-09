# Locus3D Architecture

This document contains high-level architecture notes for the planned application production code in `source/application/`.

This document is a development guide for the future runtime, native window shell, document sessions, editor viewport ownership, and application-level input routing.

For the editor interaction layer, see [`EDITOR.md`](EDITOR.md). For the graphics foundation, see [`GRAPHICS.md`](GRAPHICS.md). For the geometry/modeling kernel, see [`GEOMETRY.md`](GEOMETRY.md).

---

## Application Layer

The application layer is the outer runtime shell for Locus3D. It owns process-level startup configuration, application lifetime, frame timing, native window state, open document sessions, editor viewport composition, and routing of platform input into editor-facing events.

It should not own modeling operations, editor command history, mesh topology, or GPU resource internals. High-level tools, selection, scene editing, commands, and undo/redo should live in `source/editor/`. Geometry, topology, validation, import/export mesh logic, and manufacturing analysis should live in `source/kernel/`. Rendering, GPU resources, viewport drawing, picking, overlays, and graphics window abstractions should live in `source/graphics/`.

Conceptually:

```txt
platform / main entry point / UI shell
        |
        v
source/application runtime, windows, documents, viewports, input routing
        |
        +--> source/editor scene, tools, commands, selection, history
        |
        +--> source/graphics rendering, viewport presentation, picking, overlays
        |
        v
source/kernel geometry, modeling, validation, manufacturing analysis
```

---

## Module Responsibilities

- root files: application umbrella API, startup configuration, error/result types, and top-level application ownership.
- `runtime/`: application lifetime, startup/shutdown sequencing, per-frame execution, global application state, frame context, and frame timing.
- `window/`: application-owned window shell, persisted/restored placement, window state, and window-level actions requested by the UI or platform.
- `document/`: document identifiers, open document sessions, active document ownership, document creation/closing, and coordination between editor state and future document IO.
- `viewport/`: application-level editor viewport composition that binds a document/editor context to graphics presentation and owns viewport-scoped camera, overlay, and picking resources.
- `input/`: platform input events, current input state, capture ownership, and routing from window/input devices into editor tools and viewport interaction.

### Application Boundaries

The application layer should coordinate subsystem lifetime and data flow rather than becoming a hidden editor, renderer, or geometry backend.

Application runtime can create and tick editor, graphics, and document systems, but editor intent should remain in editor commands and tools. Application windows can host graphics windows and viewports, but low-level GPU state should remain in the graphics layer. Document sessions can own an editor scene and eventually connect to serialization, but mesh invariants and file-format-specific geometry conversion should remain in the kernel and IO boundaries.

---

## Current File Tree

Generated in the style of `tree /f` from the planned `source/application/` tree.

Legend:

- `[!]`: planned or incomplete file/directory, not present in the current tree yet.

```txt
source\application
|   CMakeLists.txt
|   Application.h
|   ApplicationConfig.h
|   ApplicationError.h
|   ApplicationResult.h
|
+---runtime
|       ApplicationRuntime.h
|       ApplicationRuntime.cpp
|       ApplicationState.h
|       FrameContext.h
|       FrameClock.h
|       FrameClock.cpp
|
+---window
|       ApplicationWindow.h
|       ApplicationWindow.cpp
|       WindowState.h [!]
|       WindowAction.h [!]
|       WindowPlacement.h [!]
|
+---document
|       DocumentId.h
|       DocumentSession.h
|       DocumentSession.cpp
|       DocumentManager.h
|       DocumentManager.cpp
|
+---viewport
|       EditorViewport.h
|       EditorViewport.cpp
|
+---shortcut
|       Shortcut.h
|       ShortcutManager.h
|       ShortcutManager.cpp
|
+---tools
|       MeshToolActivationController.h
|       MeshToolActivationController.cpp
|
\---input
        InputEvent.h
        InputState.h
        InputState.cpp
        InputCapture.h
        InputRouter.h
        InputRouter.cpp
```

---

## Planned Or Incomplete Application Files

- [!] `window/WindowState.h`
- [!] `window/WindowAction.h`
- [!] `window/WindowPlacement.h`
The remaining window-shell types should be added when persisted placement and UI-requested window actions are implemented. Editor, graphics, and kernel code should continue to expose explicit subsystem APIs without depending on application-owned global state.

---

## Notes For AI-Assisted Work

When changing application code:

- Keep process lifetime, window ownership, document sessions, and frame timing in `source/application/`.
- Keep editor behavior, command history, selection, tools, and gizmos in `source/editor/`.
- Keep GPU implementations, render scenes, overlays, viewport drawing, and picking operations in `source/graphics/`; application viewports may own their viewport-scoped graphics resource instances.
- `EditorViewport` may own viewport-scoped overlay renderers such as `PivotRenderer`, but pivot semantics remain in editor code. The viewport asks `PivotRenderAdapter` for draw data, uploads it, and renders it with the graphics renderer lifecycle.
- Keep mesh topology, modeling operations, validation, and manufacturing analysis in `source/kernel/`.
- Prefer explicit context objects over global state when connecting application runtime to editor, graphics, and document systems.
- Update this document when files are added, renamed, or promoted from planned to implemented.
