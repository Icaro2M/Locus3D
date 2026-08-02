# Locus3D Architecture

This document contains high-level architecture notes for the graphics production code in `source/graphics/`.

The graphics layer is currently the most complete production subsystem and is intended to be consumed by higher-level application, editor, and modeling code.

For the editable mesh kernel, see [`LEM.md`](LEM.md).

---

## Graphics Layer

The graphics layer is organized as a mostly independent rendering foundation. It owns low-level rendering services, GPU resource wrappers, viewport presentation, scene submission, overlays, picking, and diagnostics.

It should not own high-level editor state. Systems such as tools, selection workflows, transform gizmos, object editing, command history, and UI panels should live above this layer and call into it through the public graphics API.

Conceptually:

```txt
application/editor/tools
        |
        v
source/graphics aggregators
        |
        v
graphics modules
        |
        v
OpenGL / window / GPU resources
```

---

## Root Aggregators

The files at the root of `source/graphics/` are aggregator headers. They group related modules so higher-level code can include a stable API surface without knowing every internal path.

- `Graphics.h`: main umbrella header for the complete graphics API.
- `GraphicsCore.h`: common graphics types, errors/results, context/device access, window and input integration.
- `GraphicsGpu.h`: low-level GPU resource wrappers such as buffers, shaders, and vertex arrays.
- `GraphicsRenderer.h`: materials, cameras, lighting, mesh upload/cache, render queues, scene objects, render pipeline, and viewport state.
- `GraphicsOverlay.h`: viewport helper renderers such as grid, axis, bounding boxes, normals, and measurements.
- `GraphicsPicking.h`: object-id picking buffers, renderer, identifiers, and result types.
- `GraphicsDebug.h`: debug drawing, GPU profiling, and graphics logging helpers.

---

## Module Responsibilities

- `appearance/`: visual material definitions, material instances, built-in material factories, and viewport colors.
- `camera/`: camera state, projections, orbit-camera behavior, and viewport ray generation.
- `common/`: shared scalar aliases, color/rect types, graphics configuration, and error/result handling.
- `context/`: graphics context/device abstractions and the current OpenGL context implementation.
- `debug/`: debug draw submission, GPU profiling, and logging.
- `gpu/`: RAII-style wrappers around OpenGL resources and render state.
- `lighting/`: light data, shading modes, and the environment consumed by material-aware rendering.
- `mesh/`: upload data, GPU mesh storage, mesh uploader, draw data, and render-cache coordination.
- `overlay/`: editor/view helper rendering that sits on top of normal scene rendering.
- `passes/`: render-pass interfaces and concrete pass implementations for the render pipeline.
- `picking/`: GPU picking IDs, picking framebuffer, picking renderer, and selection result decoding.
- `renderer/`: draw commands, draw lists, render queue, render pipeline, renderer, and render statistics.
- `scene/`: renderable object state, transforms, visibility, layers, and scene containers.
- `viewport/`: viewport rectangle, clear/depth settings, camera coupling, and derived viewport state.
- `window/`: native window abstraction, cursor state, and window/input events.

---

## Current File Tree

Generated in the style of `tree /f` from `source/graphics/`.

Legend:

- `[!]`: planned or incomplete file, not present in the current tree yet.

```txt
source\graphics
|   Graphics.h
|   GraphicsCore.h
|   GraphicsDebug.h
|   GraphicsGpu.h
|   GraphicsOverlay.h
|   GraphicsPicking.h
|   GraphicsRenderer.h
|   GraphicsPrimitives.h
|
+---appearance
|       BuiltinVisualMaterials.h
|       ViewportPalette.h
|       VisualMaterial.cpp
|       VisualMaterial.h
|       VisualMaterialInstance.h
|       VisualMaterialLibrary.h
|
+---camera
|       Camera.cpp
|       Camera.h
|       CameraRayBuilder.cpp
|       CameraRayBuilder.h
|       OrbitCameraRig.cpp
|       OrbitCameraRig.h
|       Projection.cpp
|       Projection.h
|
+---common
|       GraphicsConfig.h
|       GraphicsError.h
|       GraphicsResult.h
|       GraphicsTypes.h
|
+---context
|       GraphicsCapabilities.h
|       GraphicsContext.h
|       GraphicsDevice.h
|       OpenGLContext.cpp
|       OpenGLContext.h
|
+---debug
|       DebugDraw.cpp
|       DebugDraw.h
|       GpuProfiler.cpp
|       GpuProfiler.h
|       GraphicsLogger.h
|
+---gpu
|       Buffer.cpp
|       Buffer.h
|       Framebuffer.cpp
|       Framebuffer.h
|       RenderState.cpp
|       RenderState.h
|       Shader.cpp
|       Shader.h
|       ShaderManager.cpp
|       ShaderManager.h
|       Texture.cpp
|       Texture.h
|       UniformBuffer.cpp
|       UniformBuffer.h
|       VertexArray.cpp
|       VertexArray.h
|
+---lighting
|       Light.h
|       LightEnvironment.h
|       ShadingMode.h
|
+---primitives
|       PrimitiveVertex.h
|       PrimitiveMesh.h
|       PrimitiveBuilder.h
|       PrimitiveBuilder.cpp
|       PrimitiveMeshConverter.h
|       PrimitiveMeshConverter.cpp
|       ScreenSpaceLine.h
|
+---mesh
|       GpuMesh.cpp
|       GpuMesh.h
|       MeshDrawData.h
|       MeshRenderCache.cpp
|       MeshRenderCache.h
|       MeshUploadData.h
|       MeshUploader.cpp
|       MeshUploader.h
|
+---overlay
|   \---renderers
|           AxisRenderer.cpp
|           AxisRenderer.h
|           BoundingBoxRenderer.cpp
|           BoundingBoxRenderer.h
|           GizmoRenderer.cpp
|           GizmoRenderer.h
|           GridRenderer.cpp
|           GridRenderer.h
|           MeasurementRenderer.cpp
|           MeasurementRenderer.h
|           NormalRenderer.cpp
|           NormalRenderer.h
|           ScreenSpaceLineRenderer.cpp
|           ScreenSpaceLineRenderer.h
|
+---passes
|       DebugPass.h [!]
|       GeometryPass.cpp
|       GeometryPass.h
|       IRenderPass.h
|       OutlinePass.h [!]
|       OverlayPass.h [!]
|       RenderPassContext.h
|       SelectionPass.h [!]
|       WireframePass.h [!]
|
+---picking
|       PickingBuffer.cpp
|       PickingBuffer.h
|       PickingId.h
|       PickingRenderer.cpp
|       PickingRenderer.h
|       PickingResult.h
|
+---renderer
|       DrawList.cpp
|       DrawList.h
|       RenderCommand.h
|       Renderer.cpp
|       Renderer.h
|       RenderPipeline.cpp
|       RenderPipeline.h
|       RenderQueue.cpp
|       RenderQueue.h
|       RenderStats.h
|
+---scene
|       RenderLayer.h
|       RenderObject.cpp
|       RenderObject.h
|       RenderScene.cpp
|       RenderScene.h
|       RenderTransform.cpp
|       RenderTransform.h
|       RenderVisibility.h
|
+---viewport
|       Viewport.cpp
|       Viewport.h
|       ViewportSettings.h
|       ViewportState.h
|
\---window
        Cursor.h
        Window.cpp
        Window.h
        WindowEvents.h
```

---

## Planned Or Incomplete Graphics Files

The files marked with `[!]` in the tree are likely next pieces for the graphics layer, but they are not present in the current `source/graphics/` tree yet.

- [!] `passes/WireframePass.h`
- [!] `passes/SelectionPass.h`
- [!] `passes/OutlinePass.h`
- [!] `passes/OverlayPass.h`
- [!] `passes/DebugPass.h`

They should be added only when the higher-level state they depend on becomes clear. Gizmo rendering is implemented as a graphics-only renderer that receives explicit visual state from higher layers, keeping editor concepts outside the low-level graphics layer.

---

## Notes For AI-Assisted Work

When changing graphics code:

- Prefer including the root aggregator that matches the subsystem boundary instead of reaching into many unrelated internal headers.
- Keep production changes under `source/` and production dependencies under `extern/`; the legacy MVP `src/` and `vendor/` trees have been removed.
- Keep editor/application state above `source/graphics/`.
- Treat render passes, picking, overlays, and debug helpers as graphics services that receive explicit state from higher layers.
- `ScreenSpaceLineRenderer` receives generic world-space segments only; editor topology, selection handles, and LEM details stay outside `source/graphics/`.
- Screen-space topology lines are `VisibleOnly`: they render after opaque scene geometry into the same framebuffer, keep depth testing enabled with depth writes disabled, and do not apply clip/NDC depth bias. The scene depth buffer remains the authority for occlusion, while `GL_LEQUAL` handles only coplanar numeric equality.
- `PointMarkerRenderer` follows the same overlay contract for generic world-space point markers. Markers are instanced screen-space billboards, keep the center point depth as the semantic depth for every fragment, and rely on the scene depth buffer with `GL_LEQUAL` for `VisibleOnly` occlusion.
- `SurfaceOverlayRenderer` receives generic indexed local-space triangles plus a model matrix for translucent editable-face overlays. It renders after opaque scene geometry and before topology lines/point markers, keeps depth testing enabled with depth writes disabled, uses `GL_LEQUAL` and alpha blending, and does not know editor handles, selection state, or LEM topology.
- Update this document when files are added, renamed, or promoted from planned to implemented.
