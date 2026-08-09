# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_library(locus3d_graphics STATIC)

add_library(Locus3D::Graphics ALIAS locus3d_graphics)

target_sources(locus3d_graphics PRIVATE
    "appearance/VisualMaterial.cpp"
    "camera/Camera.cpp"
    "camera/CameraRayBuilder.cpp"
    "camera/OrbitCameraRig.cpp"
    "camera/Projection.cpp"
    "context/OpenGLContext.cpp"
    "debug/DebugDraw.cpp"
    "debug/GpuProfiler.cpp"
    "gpu/Buffer.cpp"
    "gpu/Framebuffer.cpp"
    "gpu/RenderState.cpp"
    "gpu/Shader.cpp"
    "gpu/ShaderManager.cpp"
    "gpu/Texture.cpp"
    "gpu/UniformBuffer.cpp"
    "gpu/VertexArray.cpp"
    "mesh/GpuMesh.cpp"
    "mesh/MeshRenderCache.cpp"
    "mesh/MeshUploader.cpp"
    "overlay/renderers/AxisRenderer.cpp"
    "overlay/renderers/BoundingBoxRenderer.cpp"
    "overlay/renderers/GizmoRenderer.cpp"
    "overlay/renderers/GridRenderer.cpp"
    "overlay/renderers/MeasurementRenderer.cpp"
    "overlay/renderers/NormalRenderer.cpp"
    "overlay/renderers/PivotRenderer.cpp"
    "overlay/renderers/PointMarkerRenderer.cpp"
    "overlay/renderers/ScreenSpaceLineRenderer.cpp"
    "overlay/renderers/SelectionShapeRenderer.cpp"
    "overlay/renderers/SurfaceOverlayRenderer.cpp"
    "passes/GeometryPass.cpp"
    "passes/ObjectOutlinePass.cpp"
    "passes/SelectionMaskPass.cpp"
    "picking/PickingBuffer.cpp"
    "picking/PickingRenderer.cpp"
    "renderer/DrawList.cpp"
    "renderer/Renderer.cpp"
    "renderer/RenderPipeline.cpp"
    "renderer/RenderQueue.cpp"
    "scene/RenderObject.cpp"
    "scene/RenderScene.cpp"
    "scene/RenderTransform.cpp"
    "viewport/Viewport.cpp"
    "window/Window.cpp"
    "primitives/ObjectHighlight.h"
    "primitives/PrimitiveVertex.h"
    "primitives/PointMarker.h"
    "primitives/ScreenSpaceLine.h"
    "primitives/SurfaceOverlay.h"
    "primitives/PrimitiveMesh.h"
    "primitives/PrimitiveBuilder.h"
    "primitives/PrimitiveBuilder.cpp"
    "primitives/PrimitiveMeshConverter.h"
    "primitives/PrimitiveMeshConverter.cpp"
    "GraphicsPrimitives.h")

target_include_directories(locus3d_graphics PUBLIC
    ${LOCUS3D_SOURCE_DIR}
    ${GLAD_DIR}/include
    ${GLFW_DIR}/include
    ${GLM_DIR}/include
    ${NLOHMANN_DIR}
    ${STB_DIR}
)

target_link_libraries(locus3d_graphics PUBLIC
    ${GLFW_DIR}/lib/glfw3.lib
    ${LOCUS3D_PLATFORM_LIBS}
)

target_compile_definitions(locus3d_graphics PRIVATE
    _CRT_SECURE_NO_WARNINGS
)
