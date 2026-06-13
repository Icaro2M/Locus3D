# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_executable(Locus3D)

target_sources(Locus3D PRIVATE
    launcher/main.cpp
    graphics/window/Window.cpp
    graphics/context/OpenGLContext.cpp
    kernel/geometry/mesh/LEM.cpp
 "graphics/gpu/Buffer.cpp"
 "graphics/gpu/VertexArray.cpp"
 "graphics/gpu/Shader.cpp"
 "graphics/mesh/GpuMesh.cpp"
 "graphics/mesh/MeshUploader.cpp"
 "graphics/scene/RenderObject.cpp"
 "graphics/scene/RenderScene.cpp"
 "graphics/renderer/Renderer.cpp"
 "graphics/scene/RenderTransform.cpp"
 "graphics/camera/Projection.cpp"
 "graphics/camera/Camera.cpp"
 "graphics/camera/OrbitCameraRig.cpp"
 "graphics/viewport/Viewport.cpp"
 "graphics/appearance/VisualMaterial.cpp"
 "graphics/overlay/renderers/GridRenderer.cpp"
 "graphics/overlay/renderers/AxisRenderer.cpp"
 "graphics/gpu/ShaderManager.cpp"
 "graphics/renderer/RenderQueue.cpp"
 "graphics/camera/CameraRayBuilder.cpp"
 "graphics/gpu/Texture.cpp"
 "graphics/gpu/Framebuffer.cpp"
 "graphics/picking/PickingBuffer.cpp"
 "graphics/picking/PickingRenderer.cpp"
 "graphics/gpu/RenderState.cpp"
 "graphics/gpu/UniformBuffer.cpp"
 "graphics/debug/DebugDraw.cpp" 
 "graphics/overlay/renderers/BoundingBoxRenderer.cpp"
 "graphics/mesh/MeshRenderCache.cpp"
 "graphics/renderer/DrawList.cpp"
 "graphics/renderer/RenderPipeline.cpp" "graphics/passes/GeometryPass.cpp" "graphics/debug/GpuProfiler.cpp")

target_sources(Locus3D PRIVATE
    ${GLAD_DIR}/src/glad.c

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

target_include_directories(Locus3D PRIVATE
    ${LOCUS3D_SOURCE_DIR}

    ${GLAD_DIR}/include
    ${GLFW_DIR}/include
    ${GLM_DIR}/include
    ${IMGUI_DIR}
    ${NLOHMANN_DIR}
    ${STB_DIR}
)

target_link_libraries(Locus3D PRIVATE
    ${GLFW_DIR}/lib/glfw3.lib
    ${LOCUS3D_PLATFORM_LIBS}
)

target_compile_definitions(Locus3D PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

add_custom_command(TARGET Locus3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${LOCUS3D_ASSETS_DIR}"
        "$<TARGET_FILE_DIR:Locus3D>/assets"
)
