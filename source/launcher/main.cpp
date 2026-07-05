/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/SceneRenderAdapter.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/EmptyNode.h"
#include "editor/scene/MeshNode.h"
#include "graphics/scene/RenderScene.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

    bool expect(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool expect_size(
        std::size_t actual,
        std::size_t expected,
        const std::string& message)
    {
        if (actual == expected) {
            std::cout << "[OK] " << message << " = " << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << actual
            << " expected=" << expected
            << '\n';

        return false;
    }

    bool expect_u64(
        std::uint64_t actual,
        std::uint64_t expected,
        const std::string& message)
    {
        if (actual == expected) {
            std::cout << "[OK] " << message << " = " << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << actual
            << " expected=" << expected
            << '\n';

        return false;
    }

    locus::editor::SceneNodeId insert_empty_node(
        locus::editor::EditorScene& scene,
        locus::editor::SceneNodeId id,
        const std::string& name)
    {
        return scene.tree().insert_node(
            std::make_unique<locus::editor::EmptyNode>(id, name)
        );
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

    locus::kernel::geometry::FaceHandle make_triangle(
        locus::kernel::geometry::LEMEditor& editor)
    {
        const auto v0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const auto v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const auto v2 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });

        return editor.add_face(std::vector{ v0, v1, v2 });
    }

    locus::kernel::geometry::FaceHandle make_quad(
        locus::kernel::geometry::LEMEditor& editor)
    {
        const auto v0 = editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
        const auto v1 = editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
        const auto v2 = editor.add_vertex(glm::vec3{ 1.0f,  1.0f, 0.0f });
        const auto v3 = editor.add_vertex(glm::vec3{ -1.0f,  1.0f, 0.0f });

        return editor.add_face(std::vector{ v0, v1, v2, v3 });
    }

    void print_scene_result(const locus::editor::SceneRenderResult& result)
    {
        std::cout
            << "SceneRenderResult"
            << " | visited: " << result.visitedNodeCount
            << " | mesh nodes: " << result.meshNodeCount
            << " | objects: " << result.objectCount
            << " | skipped: " << result.skippedNodeCount
            << " | failed: " << result.failedNodeCount
            << " | has failures: " << (result.has_failures() ? "true" : "false")
            << '\n';

        for (const locus::editor::SceneRenderNodeResult& nodeResult : result.nodes) {
            std::cout
                << "  node " << nodeResult.nodeId.value
                << " | meshNode: " << (nodeResult.meshNode ? "true" : "false")
                << " | emitted: " << (nodeResult.emitted ? "true" : "false")
                << " | skipped: " << (nodeResult.skipped ? "true" : "false")
                << " | failed: " << (nodeResult.failed ? "true" : "false");

            if (!nodeResult.message.empty()) {
                std::cout << " | " << nodeResult.message;
            }

            std::cout << '\n';
        }
    }

    bool test_empty_scene()
    {
        using namespace locus;

        std::cout << "\n=== SceneRenderAdapter: cena vazia ===\n";

        bool ok = true;

        editor::EditorScene scene;

        editor::SceneRenderResult result{};
        const graphics::RenderScene renderScene =
            editor::SceneRenderAdapter::build_render_scene(scene, {}, {}, &result);

        print_scene_result(result);

        ok &= expect(renderScene.empty(), "render scene vazia");
        ok &= expect_size(renderScene.object_count(), 0, "object_count");
        ok &= expect_size(result.visitedNodeCount, 0, "visitedNodeCount");
        ok &= expect_size(result.meshNodeCount, 0, "meshNodeCount");
        ok &= expect_size(result.objectCount, 0, "result.objectCount");
        ok &= expect_size(result.skippedNodeCount, 0, "skippedNodeCount");
        ok &= expect_size(result.failedNodeCount, 0, "failedNodeCount");
        ok &= expect(!result.has_failures(), "sem falhas");

        return ok;
    }

    bool test_scene_with_empty_node_only()
    {
        using namespace locus;

        std::cout << "\n=== SceneRenderAdapter: cena com EmptyNode ===\n";

        bool ok = true;

        editor::EditorScene scene;

        const editor::SceneNodeId emptyId = insert_empty_node(
            scene,
            editor::SceneNodeId{ 100 },
            "Empty root"
        );

        ok &= expect(emptyId.is_valid(), "EmptyNode inserido");

        editor::SceneRenderResult result{};
        const graphics::RenderScene renderScene =
            editor::SceneRenderAdapter::build_render_scene(scene, {}, {}, &result);

        print_scene_result(result);

        ok &= expect(renderScene.empty(), "render scene vazia");
        ok &= expect_size(renderScene.object_count(), 0, "object_count");
        ok &= expect_size(result.visitedNodeCount, 1, "visitedNodeCount");
        ok &= expect_size(result.meshNodeCount, 0, "meshNodeCount");
        ok &= expect_size(result.objectCount, 0, "result.objectCount");
        ok &= expect_size(result.skippedNodeCount, 1, "skippedNodeCount");
        ok &= expect_size(result.failedNodeCount, 0, "failedNodeCount");
        ok &= expect(!result.has_failures(), "sem falhas");
        ok &= expect_size(result.nodes.size(), 1, "node result count");
        ok &= expect(result.nodes[0].skipped, "EmptyNode foi pulado");
        ok &= expect(!result.nodes[0].meshNode, "EmptyNode nao contado como mesh");

        return ok;
    }

    bool test_scene_with_visible_mesh()
    {
        using namespace locus;

        std::cout << "\n=== SceneRenderAdapter: cena com MeshNode visivel ===\n";

        bool ok = true;

        editor::EditorScene scene;

        const editor::SceneNodeId meshId = insert_mesh_node(
            scene,
            editor::SceneNodeId{ 200 },
            "Visible mesh"
        );

        ok &= expect(meshId.is_valid(), "MeshNode inserido");

        editor::MeshNode* meshNode = scene.find_mesh(meshId);
        ok &= expect(meshNode != nullptr, "MeshNode encontrado por find_mesh");

        if (!meshNode) {
            return false;
        }

        kernel::geometry::LEMEditor meshEditor{ meshNode->mesh() };
        const auto face = make_triangle(meshEditor);

        ok &= expect(meshNode->mesh().is_valid(face), "face triangular criada");

        meshNode->transform().set_position(glm::vec3{ 2.0f, 3.0f, 4.0f });
        meshNode->metadata().visible = true;
        meshNode->metadata().selectable = true;
        meshNode->metadata().locked = false;

        editor::SceneRenderOptions options{};
        options.includeHiddenNodes = true;
        options.allowNullGpuMeshes = true;
        options.meshOptions.selected = true;
        options.meshOptions.hovered = true;
        options.meshOptions.wireframe = false;

        editor::SceneRenderResult result{};
        const graphics::RenderScene renderScene =
            editor::SceneRenderAdapter::build_render_scene(scene, {}, options, &result);

        print_scene_result(result);

        ok &= expect(!renderScene.empty(), "render scene nao vazia");
        ok &= expect_size(renderScene.object_count(), 1, "object_count");
        ok &= expect_size(result.visitedNodeCount, 1, "visitedNodeCount");
        ok &= expect_size(result.meshNodeCount, 1, "meshNodeCount");
        ok &= expect_size(result.objectCount, 1, "result.objectCount");
        ok &= expect_size(result.skippedNodeCount, 0, "skippedNodeCount");
        ok &= expect_size(result.failedNodeCount, 0, "failedNodeCount");
        ok &= expect(!result.has_failures(), "sem falhas");

        const graphics::RenderObject& object = renderScene.objects().front();

        ok &= expect_u64(object.id, 200, "object.id");
        ok &= expect(object.name == "Visible mesh", "object.name preservado");
        ok &= expect(object.mesh == nullptr, "object.mesh nullptr permitido no teste sem GPU");
        ok &= expect(object.visibility.visible, "object visible true");
        ok &= expect(object.visibility.selectable, "object selectable true");
        ok &= expect(object.selected, "object selected true");
        ok &= expect(object.hovered, "object hovered true");
        ok &= expect(!object.wireframe, "object wireframe false");
        ok &= expect(object.transform.position.x == 2.0f, "position.x preservada");
        ok &= expect(object.transform.position.y == 3.0f, "position.y preservada");
        ok &= expect(object.transform.position.z == 4.0f, "position.z preservada");

        ok &= expect_size(result.nodes.size(), 1, "node result count");
        ok &= expect(result.nodes[0].meshNode, "node result marcado como mesh");
        ok &= expect(result.nodes[0].emitted, "node result emitido");
        ok &= expect(!result.nodes[0].skipped, "node result nao pulado");
        ok &= expect(!result.nodes[0].failed, "node result sem falha");

        return ok;
    }

    bool test_hidden_mesh_included()
    {
        using namespace locus;

        std::cout << "\n=== SceneRenderAdapter: MeshNode invisivel incluido ===\n";

        bool ok = true;

        editor::EditorScene scene;

        const editor::SceneNodeId meshId = insert_mesh_node(
            scene,
            editor::SceneNodeId{ 300 },
            "Hidden mesh included"
        );

        ok &= expect(meshId.is_valid(), "MeshNode inserido");

        editor::MeshNode* meshNode = scene.find_mesh(meshId);
        ok &= expect(meshNode != nullptr, "MeshNode encontrado por find_mesh");

        if (!meshNode) {
            return false;
        }

        kernel::geometry::LEMEditor meshEditor{ meshNode->mesh() };
        const auto face = make_quad(meshEditor);

        ok &= expect(meshNode->mesh().is_valid(face), "face quad criada");

        meshNode->metadata().visible = false;
        meshNode->metadata().selectable = true;
        meshNode->metadata().locked = false;

        editor::SceneRenderOptions options{};
        options.includeHiddenNodes = true;
        options.allowNullGpuMeshes = true;

        editor::SceneRenderResult result{};
        const graphics::RenderScene renderScene =
            editor::SceneRenderAdapter::build_render_scene(scene, {}, options, &result);

        print_scene_result(result);

        ok &= expect_size(renderScene.object_count(), 1, "object_count");
        ok &= expect_size(result.visitedNodeCount, 1, "visitedNodeCount");
        ok &= expect_size(result.meshNodeCount, 1, "meshNodeCount");
        ok &= expect_size(result.objectCount, 1, "result.objectCount");
        ok &= expect_size(result.skippedNodeCount, 0, "skippedNodeCount");
        ok &= expect_size(result.failedNodeCount, 0, "failedNodeCount");

        const graphics::RenderObject& object = renderScene.objects().front();

        ok &= expect(!object.visibility.visible, "object visible false");
        ok &= expect(!object.visibility.selectable, "object selectable false por invisivel");

        return ok;
    }

    bool test_hidden_mesh_skipped()
    {
        using namespace locus;

        std::cout << "\n=== SceneRenderAdapter: MeshNode invisivel pulado ===\n";

        bool ok = true;

        editor::EditorScene scene;

        const editor::SceneNodeId meshId = insert_mesh_node(
            scene,
            editor::SceneNodeId{ 400 },
            "Hidden mesh skipped"
        );

        ok &= expect(meshId.is_valid(), "MeshNode inserido");

        editor::MeshNode* meshNode = scene.find_mesh(meshId);
        ok &= expect(meshNode != nullptr, "MeshNode encontrado por find_mesh");

        if (!meshNode) {
            return false;
        }

        kernel::geometry::LEMEditor meshEditor{ meshNode->mesh() };
        const auto face = make_triangle(meshEditor);

        ok &= expect(meshNode->mesh().is_valid(face), "face triangular criada");

        meshNode->metadata().visible = false;
        meshNode->metadata().selectable = true;
        meshNode->metadata().locked = false;

        editor::SceneRenderOptions options{};
        options.includeHiddenNodes = false;
        options.allowNullGpuMeshes = true;

        editor::SceneRenderResult result{};
        const graphics::RenderScene renderScene =
            editor::SceneRenderAdapter::build_render_scene(scene, {}, options, &result);

        print_scene_result(result);

        ok &= expect(renderScene.empty(), "render scene vazia");
        ok &= expect_size(renderScene.object_count(), 0, "object_count");
        ok &= expect_size(result.visitedNodeCount, 1, "visitedNodeCount");
        ok &= expect_size(result.meshNodeCount, 1, "meshNodeCount");
        ok &= expect_size(result.objectCount, 0, "result.objectCount");
        ok &= expect_size(result.skippedNodeCount, 1, "skippedNodeCount");
        ok &= expect_size(result.failedNodeCount, 0, "failedNodeCount");
        ok &= expect_size(result.nodes.size(), 1, "node result count");
        ok &= expect(result.nodes[0].skipped, "node result pulado");
        ok &= expect(!result.nodes[0].emitted, "node result nao emitido");

        return ok;
    }

    bool test_null_gpu_mesh_disallowed()
    {
        using namespace locus;

        std::cout << "\n=== SceneRenderAdapter: nullptr GpuMesh nao permitido ===\n";

        bool ok = true;

        editor::EditorScene scene;

        const editor::SceneNodeId meshId = insert_mesh_node(
            scene,
            editor::SceneNodeId{ 500 },
            "Mesh without GPU"
        );

        ok &= expect(meshId.is_valid(), "MeshNode inserido");

        editor::MeshNode* meshNode = scene.find_mesh(meshId);
        ok &= expect(meshNode != nullptr, "MeshNode encontrado por find_mesh");

        if (!meshNode) {
            return false;
        }

        kernel::geometry::LEMEditor meshEditor{ meshNode->mesh() };
        const auto face = make_triangle(meshEditor);

        ok &= expect(meshNode->mesh().is_valid(face), "face triangular criada");

        editor::SceneRenderOptions options{};
        options.includeHiddenNodes = true;
        options.allowNullGpuMeshes = false;

        editor::SceneRenderResult result{};
        const graphics::RenderScene renderScene =
            editor::SceneRenderAdapter::build_render_scene(scene, {}, options, &result);

        print_scene_result(result);

        ok &= expect(renderScene.empty(), "render scene vazia");
        ok &= expect_size(renderScene.object_count(), 0, "object_count");
        ok &= expect_size(result.visitedNodeCount, 1, "visitedNodeCount");
        ok &= expect_size(result.meshNodeCount, 1, "meshNodeCount");
        ok &= expect_size(result.objectCount, 0, "result.objectCount");
        ok &= expect_size(result.skippedNodeCount, 1, "skippedNodeCount");
        ok &= expect_size(result.failedNodeCount, 0, "failedNodeCount");
        ok &= expect_size(result.nodes.size(), 1, "node result count");
        ok &= expect(result.nodes[0].skipped, "node result pulado por falta de GPU mesh");
        ok &= expect(!result.nodes[0].failed, "falta de GPU mesh tratada como skip, nao falha");

        return ok;
    }

    bool test_mixed_scene()
    {
        using namespace locus;

        std::cout << "\n=== SceneRenderAdapter: cena mista ===\n";

        bool ok = true;

        editor::EditorScene scene;

        const editor::SceneNodeId emptyId = insert_empty_node(
            scene,
            editor::SceneNodeId{ 600 },
            "Group"
        );

        const editor::SceneNodeId meshAId = insert_mesh_node(
            scene,
            editor::SceneNodeId{ 601 },
            "Mesh A"
        );

        const editor::SceneNodeId meshBId = insert_mesh_node(
            scene,
            editor::SceneNodeId{ 602 },
            "Mesh B hidden"
        );

        ok &= expect(emptyId.is_valid(), "EmptyNode inserido");
        ok &= expect(meshAId.is_valid(), "MeshNode A inserido");
        ok &= expect(meshBId.is_valid(), "MeshNode B inserido");

        editor::MeshNode* meshA = scene.find_mesh(meshAId);
        editor::MeshNode* meshB = scene.find_mesh(meshBId);

        ok &= expect(meshA != nullptr, "MeshNode A encontrado");
        ok &= expect(meshB != nullptr, "MeshNode B encontrado");

        if (!meshA || !meshB) {
            return false;
        }

        {
            kernel::geometry::LEMEditor meshEditor{ meshA->mesh() };
            ok &= expect(meshA->mesh().is_valid(make_triangle(meshEditor)), "Mesh A triangulo criado");
        }

        {
            kernel::geometry::LEMEditor meshEditor{ meshB->mesh() };
            ok &= expect(meshB->mesh().is_valid(make_quad(meshEditor)), "Mesh B quad criado");
        }

        meshA->metadata().visible = true;
        meshA->metadata().selectable = true;
        meshA->metadata().locked = false;

        meshB->metadata().visible = false;
        meshB->metadata().selectable = true;
        meshB->metadata().locked = false;

        editor::SceneRenderOptions options{};
        options.includeHiddenNodes = false;
        options.allowNullGpuMeshes = true;

        editor::SceneRenderResult result{};
        const graphics::RenderScene renderScene =
            editor::SceneRenderAdapter::build_render_scene(scene, {}, options, &result);

        print_scene_result(result);

        ok &= expect_size(scene.tree().size(), 3, "scene tree size");
        ok &= expect_size(renderScene.object_count(), 1, "object_count");
        ok &= expect_size(result.visitedNodeCount, 3, "visitedNodeCount");
        ok &= expect_size(result.meshNodeCount, 2, "meshNodeCount");
        ok &= expect_size(result.objectCount, 1, "result.objectCount");
        ok &= expect_size(result.skippedNodeCount, 2, "skippedNodeCount");
        ok &= expect_size(result.failedNodeCount, 0, "failedNodeCount");
        ok &= expect(!result.has_failures(), "sem falhas");

        const graphics::RenderObject& object = renderScene.objects().front();

        ok &= expect_u64(object.id, 601, "objeto emitido corresponde ao Mesh A");
        ok &= expect(object.name == "Mesh A", "nome do objeto emitido");

        return ok;
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor SceneRenderAdapter Smoke Test ===\n";

    bool ok = true;

    ok &= test_empty_scene();
    ok &= test_scene_with_empty_node_only();
    ok &= test_scene_with_visible_mesh();
    ok &= test_hidden_mesh_included();
    ok &= test_hidden_mesh_skipped();
    ok &= test_null_gpu_mesh_disallowed();
    ok &= test_mixed_scene();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de SceneRenderAdapter passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de SceneRenderAdapter falhou.\n";
    return EXIT_FAILURE;
}