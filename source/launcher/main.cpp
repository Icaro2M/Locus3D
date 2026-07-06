/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/scene/MeshNode.h"
#include "editor/sync/EditorSync.h"

#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/MeshRenderCache.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/viewport/Viewport.h"
#include "graphics/window/Window.h"

#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/vec3.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

    constexpr const char* MeshVertexShader = R"glsl(
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 v_Normal;
out vec4 v_Color;

void main()
{
    v_Normal = mat3(u_Model) * a_Normal;
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)glsl";

    constexpr const char* MeshFragmentShader = R"glsl(
#version 450 core

in vec3 v_Normal;
in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(v_Normal);
    float light = max(dot(normal, normalize(vec3(0.35, 0.55, 1.0))), 0.0);
    float shade = 0.35 + light * 0.65;

    vec4 baseColor = (u_UseVertexColor != 0) ? v_Color : u_BaseColor;
    FragColor = vec4(baseColor.rgb * shade, baseColor.a);
}
)glsl";

    bool expect(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    void print_graphics_error(
        const std::string& label,
        const locus::graphics::GraphicsError& error)
    {
        std::cout
            << "[FAIL] " << label
            << " | code=" << static_cast<int>(error.code)
            << " | message=" << error.message
            << '\n';
    }

    locus::editor::SceneNodeId insert_mesh_node(
        locus::editor::EditorScene& scene,
        locus::editor::SceneNodeId id,
        const std::string& name)
    {
        return scene.tree().insert_node(
            std::make_unique<locus::editor::MeshNode>(id, name)
        );
    }

    locus::kernel::geometry::FaceHandle make_quad(
        locus::kernel::geometry::LEMEditor& editor,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec3& d)
    {
        const auto v0 = editor.add_vertex(a);
        const auto v1 = editor.add_vertex(b);
        const auto v2 = editor.add_vertex(c);
        const auto v3 = editor.add_vertex(d);

        return editor.add_face(std::vector{ v0, v1, v2, v3 });
    }

    locus::kernel::geometry::FaceHandle make_triangle(
        locus::kernel::geometry::LEMEditor& editor,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c)
    {
        const auto v0 = editor.add_vertex(a);
        const auto v1 = editor.add_vertex(b);
        const auto v2 = editor.add_vertex(c);

        return editor.add_face(std::vector{ v0, v1, v2 });
    }

    bool build_editor_scene(locus::editor::Editor& editor)
    {
        using namespace locus;

        bool ok = true;

        const editor::SceneNodeId quadId =
            insert_mesh_node(editor.scene(), editor::SceneNodeId{ 100 }, "Editor quad");

        const editor::SceneNodeId triangleId =
            insert_mesh_node(editor.scene(), editor::SceneNodeId{ 200 }, "Editor triangle");

        ok &= expect(quadId.is_valid(), "quad MeshNode inserido");
        ok &= expect(triangleId.is_valid(), "triangle MeshNode inserido");

        editor::MeshNode* quadNode = editor.scene().find_mesh(quadId);
        editor::MeshNode* triangleNode = editor.scene().find_mesh(triangleId);

        ok &= expect(quadNode != nullptr, "quad MeshNode encontrado");
        ok &= expect(triangleNode != nullptr, "triangle MeshNode encontrado");

        if (!quadNode || !triangleNode) {
            return false;
        }

        {
            kernel::geometry::LEMEditor meshEditor{ quadNode->mesh() };

            const auto face = make_quad(
                meshEditor,
                glm::vec3{ -0.8f, -0.8f, 0.0f },
                glm::vec3{ 0.8f, -0.8f, 0.0f },
                glm::vec3{ 0.8f,  0.8f, 0.0f },
                glm::vec3{ -0.8f,  0.8f, 0.0f }
            );

            ok &= expect(quadNode->mesh().is_valid(face), "quad face criada");
            quadNode->transform().set_position(glm::vec3{ -1.2f, 0.0f, 0.0f });
        }

        {
            kernel::geometry::LEMEditor meshEditor{ triangleNode->mesh() };

            const auto face = make_triangle(
                meshEditor,
                glm::vec3{ 0.0f,  0.9f, 0.0f },
                glm::vec3{ -0.9f, -0.7f, 0.0f },
                glm::vec3{ 0.9f, -0.7f, 0.0f }
            );

            ok &= expect(triangleNode->mesh().is_valid(face), "triangle face criada");
            triangleNode->transform().set_position(glm::vec3{ 1.2f, 0.0f, 0.0f });
        }

        quadNode->metadata().visible = true;
        quadNode->metadata().selectable = true;
        quadNode->metadata().locked = false;

        triangleNode->metadata().visible = true;
        triangleNode->metadata().selectable = true;
        triangleNode->metadata().locked = false;

        editor.selection().objects().set(quadId);
        editor.selection().objects().set_hovered(triangleId);

        editor.mark_dirty(
            editor::EditorDirtyFlags::Scene |
            editor::EditorDirtyFlags::Mesh |
            editor::EditorDirtyFlags::Selection |
            editor::EditorDirtyFlags::Render
        );

        return ok;
    }

    bool print_sync_summary(const locus::editor::EditorSync& sync)
    {
        const locus::editor::EditorSyncResult& result = sync.last_result();

        std::cout
            << "EditorSyncResult"
            << " | renderSceneSynced: " << (result.renderSceneSynced ? "true" : "false")
            << " | dirtyFlagsCleared: " << (result.dirtyFlagsCleared ? "true" : "false")
            << " | objectCount: " << result.renderSceneResult.objectCount
            << '\n';

        std::cout
            << "RenderSceneSyncResult"
            << " | rebuilt: " << (result.renderSceneResult.rebuilt ? "true" : "false")
            << " | selectionApplied: " << (result.renderSceneResult.selectionApplied ? "true" : "false")
            << " | usedGpuCache: " << (result.renderSceneResult.usedGpuCache ? "true" : "false")
            << " | sceneObjects: " << sync.render_scene().object_count()
            << '\n';

        std::cout
            << "SceneRenderResult"
            << " | visited: " << result.renderSceneResult.sceneResult.visitedNodeCount
            << " | meshNodes: " << result.renderSceneResult.sceneResult.meshNodeCount
            << " | objects: " << result.renderSceneResult.sceneResult.objectCount
            << " | skipped: " << result.renderSceneResult.sceneResult.skippedNodeCount
            << " | failed: " << result.renderSceneResult.sceneResult.failedNodeCount
            << '\n';

        std::cout
            << "SelectionRenderResult"
            << " | visited: " << result.renderSceneResult.selectionResult.visitedObjectCount
            << " | selected: " << result.renderSceneResult.selectionResult.selectedObjectCount
            << " | hovered: " << result.renderSceneResult.selectionResult.hoveredObjectCount
            << " | changed: " << result.renderSceneResult.selectionResult.changedObjectCount
            << '\n';

        if (!result.message.empty()) {
            std::cout << "Message: " << result.message << '\n';
        }

        return result.renderSceneResult.objectCount > 0;
    }

} // namespace

int main()
{
    using namespace locus;

    std::cout << "=== Locus3D EditorSync GPU Visual Test ===\n";

    graphics::WindowCreateInfo windowInfo{};
    windowInfo.width = 1280;
    windowInfo.height = 720;
    windowInfo.title = "Locus3D - EditorSync GPU Visual Test";
    windowInfo.resizable = true;
    windowInfo.visible = true;
    windowInfo.requestOpenGLContext = true;
    windowInfo.openglMajorVersion = 4;
    windowInfo.openglMinorVersion = 5;
    windowInfo.openglCoreProfile = true;
    windowInfo.openglDebugContext = true;

    graphics::Window window;
    auto windowResult = window.create(windowInfo);

    if (!windowResult) {
        print_graphics_error("Window creation failed", windowResult.error());
        return EXIT_FAILURE;
    }

    graphics::GraphicsConfig graphicsConfig{};
    graphicsConfig.enableDebugOutput = true;
    graphicsConfig.enableVSync = true;
    graphicsConfig.requestedMajorVersion = 4;
    graphicsConfig.requestedMinorVersion = 5;
    graphicsConfig.coreProfile = true;
    graphicsConfig.forwardCompatible = true;
    graphicsConfig.defaultClearColor = graphics::ColorRGBA{ 0.06f, 0.065f, 0.075f, 1.0f };

    graphics::OpenGLContext context;
    auto contextResult = context.initialize(window, graphicsConfig);

    if (!contextResult) {
        print_graphics_error("OpenGL context initialization failed", contextResult.error());
        return EXIT_FAILURE;
    }

    context.set_vsync(true);

    std::cout << "OpenGL Vendor: " << context.capabilities().vendor << '\n';
    std::cout << "OpenGL Renderer: " << context.capabilities().renderer << '\n';
    std::cout << "OpenGL Version: " << context.capabilities().version << '\n';
    std::cout << "GLSL Version: " << context.capabilities().shadingLanguageVersion << '\n';

    graphics::Shader shader;
    auto shaderResult = shader.create_from_source(MeshVertexShader, MeshFragmentShader);

    if (!shaderResult) {
        print_graphics_error("Shader creation failed", shaderResult.error());
        return EXIT_FAILURE;
    }

    graphics::Viewport viewport;
    viewport.sync_with_window(window);
    viewport.set_clear_color(graphicsConfig.defaultClearColor);
    viewport.set_depth_test_enabled(true);
    viewport.camera().look_at(
        glm::vec3{ 0.0f, 0.0f, 4.5f },
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 1.0f, 0.0f }
    );

    graphics::Renderer renderer;
    graphics::MeshUploader uploader;
    graphics::MeshRenderCache meshCache;

    editor::Editor editorFacade;

    if (!build_editor_scene(editorFacade)) {
        std::cout << "[FAIL] Nao foi possivel construir a EditorScene de teste.\n";
        return EXIT_FAILURE;
    }

    editor::EditorSync sync;

    editor::EditorSyncOptions syncOptions{};
    syncOptions.clearDirtyFlagsAfterSync = true;
    syncOptions.renderSceneOptions.applySelection = true;
    syncOptions.renderSceneOptions.sceneOptions.includeHiddenNodes = true;
    syncOptions.renderSceneOptions.sceneOptions.meshOptions.shader = &shader;
    syncOptions.renderSceneOptions.sceneOptions.meshOptions.uploadOptions.color =
        graphics::ColorRGBA{ 0.25f, 0.70f, 1.0f, 1.0f };
    syncOptions.renderSceneOptions.sceneOptions.fallbackMeshRevision = 1;

    auto syncResult =
        sync.sync_cached_if_needed(editorFacade, meshCache, uploader, syncOptions);

    if (!syncResult) {
        print_graphics_error("EditorSync cached path failed", syncResult.error());
        return EXIT_FAILURE;
    }

    if (!print_sync_summary(sync)) {
        std::cout << "[FAIL] Sync nao gerou objetos renderizaveis.\n";
        return EXIT_FAILURE;
    }

    std::cout << "\nControles: feche a janela para encerrar.\n";
    std::cout << "Esperado: um quad azul/ciano à esquerda e um triangulo azul/ciano à direita.\n\n";

    bool printedFirstFrameStats = false;

    while (!window.should_close()) {
        window.poll_events();

        viewport.sync_with_window(window);
        viewport.begin_frame();

        renderer.set_view_matrix(viewport.camera().view_matrix());
        renderer.set_projection_matrix(viewport.camera().projection_matrix());
        renderer.render(sync.render_scene());

        if (!printedFirstFrameStats) {
            const graphics::RenderStats& stats = renderer.stats();

            std::cout
                << "First frame RenderStats"
                << " | submitted: " << stats.objectsSubmitted
                << " | drawn: " << stats.objectsDrawn
                << " | skipped: " << stats.objectsSkipped
                << " | drawCalls: " << stats.drawCalls
                << '\n';

            printedFirstFrameStats = true;
        }

        window.swap_buffers();

        std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
    }

    meshCache.clear();
    shader.destroy();
    context.shutdown();
    window.destroy();

    std::cout << "\n[OK] EditorSync GPU Visual Test encerrado.\n";
    return EXIT_SUCCESS;
}   