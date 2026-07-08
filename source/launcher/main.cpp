/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/snapping/EdgeSnapProvider.h"
#include "editor/snapping/GridSnapProvider.h"
#include "editor/snapping/SnapContext.h"
#include "editor/snapping/SnapMode.h"
#include "editor/snapping/SnapResult.h"
#include "editor/snapping/SnapSolver.h"
#include "editor/snapping/SnapTarget.h"
#include "editor/snapping/VertexSnapProvider.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include <glm/glm.hpp>

namespace {

    constexpr float kEpsilon = 0.0001f;

    bool almost_equal(float lhs, float rhs, float epsilon = kEpsilon)
    {
        const float diff = lhs - rhs;
        return diff < epsilon && diff > -epsilon;
    }

    bool almost_equal_vec3(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        float epsilon = kEpsilon)
    {
        return almost_equal(lhs.x, rhs.x, epsilon)
            && almost_equal(lhs.y, rhs.y, epsilon)
            && almost_equal(lhs.z, rhs.z, epsilon);
    }

    bool snap_mode_equals(
        locus::editor::SnapMode actual,
        locus::editor::SnapMode expected)
    {
        return static_cast<std::uint32_t>(actual) == static_cast<std::uint32_t>(expected);
    }

    bool expect(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool expect_vec3(
        const glm::vec3& actual,
        const glm::vec3& expected,
        const std::string& message,
        float epsilon = kEpsilon)
    {
        if (almost_equal_vec3(actual, expected, epsilon)) {
            std::cout
                << "[OK] " << message
                << " = (" << actual.x << ", " << actual.y << ", " << actual.z << ")"
                << '\n';

            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=(" << actual.x << ", " << actual.y << ", " << actual.z << ")"
            << " expected=(" << expected.x << ", " << expected.y << ", " << expected.z << ")"
            << '\n';

        return false;
    }

    const char* snap_target_type_name(locus::editor::SnapTargetType type)
    {
        using locus::editor::SnapTargetType;

        switch (type) {
        case SnapTargetType::None:
            return "None";
        case SnapTargetType::GridPoint:
            return "GridPoint";
        case SnapTargetType::Vertex:
            return "Vertex";
        case SnapTargetType::Edge:
            return "Edge";
        case SnapTargetType::Face:
            return "Face";
        case SnapTargetType::Increment:
            return "Increment";
        case SnapTargetType::Angle:
            return "Angle";
        }

        return "Unknown";
    }

    void print_result(const locus::editor::SnapResult& result)
    {
        std::cout
            << "SnapResult"
            << " | valid: " << (result.is_valid() ? "true" : "false")
            << " | target: " << snap_target_type_name(result.target.type)
            << " | node valid: " << (result.target.node.is_valid() ? "true" : "false")
            << " | component: " << result.target.component
            << " | candidate: ("
            << result.candidatePosition.x << ", "
            << result.candidatePosition.y << ", "
            << result.candidatePosition.z << ")"
            << " | snapped: ("
            << result.snappedPosition.x << ", "
            << result.snappedPosition.y << ", "
            << result.snappedPosition.z << ")"
            << " | normal/dir: ("
            << result.target.normal.x << ", "
            << result.target.normal.y << ", "
            << result.target.normal.z << ")"
            << " | distance: " << result.distance
            << " | score: " << result.score
            << '\n';
    }

    locus::editor::SnapContext make_context(
        const locus::editor::EditorScene& scene,
        const glm::vec3& original,
        const glm::vec3& candidate,
        const glm::vec3& referenceOrigin = glm::vec3{ 0.0f, 0.0f, 0.0f })
    {
        locus::editor::SnapContext context{};
        context.scene = &scene;
        context.originalPosition = original;
        context.candidatePosition = candidate;
        context.referenceOrigin = referenceOrigin;
        return context;
    }

    bool test_edge_provider_without_scene()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: sem scene ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Edge);
        settings.set_max_distance(10.0f);

        editor::EdgeSnapProvider provider;

        editor::SnapContext context{};
        context.scene = nullptr;
        context.originalPosition = { 0.0f, 0.0f, 0.0f };
        context.candidatePosition = { 1.0f, 1.0f, 1.0f };

        ok &= expect(
            !provider.is_enabled(settings, context),
            "provider nao fica habilitado sem scene");

        const editor::SnapResult result = provider.snap(settings, context);
        print_result(result);

        ok &= expect(!result.is_valid(), "snap sem scene retorna invalid");

        return ok;
    }

    bool test_edge_provider_empty_scene()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: scene vazia ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f });

        ok &= expect(
            provider.is_enabled(editor.snap_settings(), context),
            "provider habilitado com scene valida");

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(!result.is_valid(), "scene vazia retorna invalid");

        return ok;
    }

    bool test_edge_provider_mesh_without_edges()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: mesh sem arestas ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Sem Aresta");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        meshNode->mesh().add_vertex({ 2.0f, 0.0f, 0.0f });

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 0.2f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(!result.is_valid(), "vertices soltos sem edge retornam invalid");

        return ok;
    }

    bool test_edge_provider_single_edge_midpoint()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: aresta simples ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Edge");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshId.is_valid(), "mesh node criado com id valido");
        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        const kernel::geometry::EdgeHandle e0 = meshNode->mesh().find_or_create_edge(v0, v1);

        ok &= expect(v0.is_valid(), "v0 criado");
        ok &= expect(v1.is_valid(), "v1 criado");
        ok &= expect(e0.is_valid(), "edge criada");

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.2f, 0.7f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "edge snap encontrou resultado");
        ok &= expect(snap_mode_equals(result.mode, editor::SnapMode::Edge), "modo do resultado eh Edge");
        ok &= expect(result.target.type == editor::SnapTargetType::Edge, "target eh Edge");
        ok &= expect(result.target.node == meshId, "target aponta para mesh correta");
        ok &= expect(result.target.component == 0u, "target component aponta para edge 0");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 2.2f, 0.0f, 0.0f }, "snapped no ponto mais proximo da aresta");
        ok &= expect_vec3(result.target.normal, glm::vec3{ 1.0f, 0.0f, 0.0f }, "direcao da aresta em mundo");

        return ok;
    }

    bool test_edge_provider_clamps_to_segment_start()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: clamp inicio do segmento ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Clamp Start");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        meshNode->mesh().find_or_create_edge(v0, v1);

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ -1.0f, 0.5f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "edge snap encontrou resultado");
        ok &= expect(result.target.node == meshId, "target aponta para mesh correta");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 0.0f, 0.0f, 0.0f }, "snap clampou no inicio do segmento");

        return ok;
    }

    bool test_edge_provider_clamps_to_segment_end()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: clamp fim do segmento ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Clamp End");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        meshNode->mesh().find_or_create_edge(v0, v1);

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 5.0f, 0.5f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "edge snap encontrou resultado");
        ok &= expect(result.target.node == meshId, "target aponta para mesh correta");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 4.0f, 0.0f, 0.0f }, "snap clampou no fim do segmento");

        return ok;
    }

    bool test_edge_provider_world_transform()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: transform local -> mundo ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Edge Transformada");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh transformada encontrada");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        meshNode->mesh().find_or_create_edge(v0, v1);
        meshNode->transform().set_position({ 10.0f, 5.0f, -2.0f });

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 12.5f, 5.8f, -2.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "edge snap com transform encontrou resultado");
        ok &= expect(result.target.node == meshId, "target aponta para mesh transformada");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 12.5f, 5.0f, -2.0f }, "snapped em coordenada de mundo");
        ok &= expect_vec3(result.target.normal, glm::vec3{ 1.0f, 0.0f, 0.0f }, "direcao da aresta transformada");

        return ok;
    }

    bool test_edge_provider_chooses_nearest_edge()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: escolhe aresta mais proxima ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Multi Edge");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh multi edge encontrada");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle a0 = meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle a1 = meshNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle b0 = meshNode->mesh().add_vertex({ 0.0f, 3.0f, 0.0f });
        const kernel::geometry::VertexHandle b1 = meshNode->mesh().add_vertex({ 4.0f, 3.0f, 0.0f });

        meshNode->mesh().find_or_create_edge(a0, a1);
        meshNode->mesh().find_or_create_edge(b0, b1);

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.0f, 2.7f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "edge snap encontrou resultado");
        ok &= expect(result.target.node == meshId, "target aponta para mesh correta");
        ok &= expect(result.target.component == 1u, "target component aponta para segunda edge");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 2.0f, 3.0f, 0.0f }, "snapped na aresta mais proxima");

        return ok;
    }

    bool test_edge_provider_ignores_invisible_node()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: ignora node invisivel ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId visibleId = editor.scene().create_mesh("Visible Edge Mesh");
        const editor::SceneNodeId invisibleId = editor.scene().create_mesh("Invisible Edge Mesh");

        editor::MeshNode* visibleNode = editor.scene().find_mesh(visibleId);
        editor::MeshNode* invisibleNode = editor.scene().find_mesh(invisibleId);

        ok &= expect(visibleNode != nullptr, "visible node encontrado");
        ok &= expect(invisibleNode != nullptr, "invisible node encontrado");

        if (visibleNode == nullptr || invisibleNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle va0 = visibleNode->mesh().add_vertex({ 5.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle va1 = visibleNode->mesh().add_vertex({ 9.0f, 0.0f, 0.0f });
        visibleNode->mesh().find_or_create_edge(va0, va1);

        const kernel::geometry::VertexHandle vb0 = invisibleNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle vb1 = invisibleNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        invisibleNode->mesh().find_or_create_edge(vb0, vb1);

        invisibleNode->metadata().visible = false;

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.0f, 0.1f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "snap encontrou apenas node visivel");
        ok &= expect(result.target.node == visibleId, "node invisivel foi ignorado");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 5.0f, 0.0f, 0.0f }, "snapped na edge visivel restante");

        return ok;
    }

    bool test_edge_provider_ignores_unselectable_node()
    {
        using namespace locus;

        std::cout << "\n=== EdgeSnapProvider: ignora node nao selecionavel ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId selectableId = editor.scene().create_mesh("Selectable Edge Mesh");
        const editor::SceneNodeId unselectableId = editor.scene().create_mesh("Unselectable Edge Mesh");

        editor::MeshNode* selectableNode = editor.scene().find_mesh(selectableId);
        editor::MeshNode* unselectableNode = editor.scene().find_mesh(unselectableId);

        ok &= expect(selectableNode != nullptr, "selectable node encontrado");
        ok &= expect(unselectableNode != nullptr, "unselectable node encontrado");

        if (selectableNode == nullptr || unselectableNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle va0 = selectableNode->mesh().add_vertex({ 5.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle va1 = selectableNode->mesh().add_vertex({ 9.0f, 0.0f, 0.0f });
        selectableNode->mesh().find_or_create_edge(va0, va1);

        const kernel::geometry::VertexHandle vb0 = unselectableNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle vb1 = unselectableNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        unselectableNode->mesh().find_or_create_edge(vb0, vb1);

        unselectableNode->metadata().selectable = false;

        editor::EdgeSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.0f, 0.1f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "snap encontrou apenas node selecionavel");
        ok &= expect(result.target.node == selectableId, "node nao selecionavel foi ignorado");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 5.0f, 0.0f, 0.0f }, "snapped na edge selecionavel restante");

        return ok;
    }

    bool test_solver_edge_beats_grid()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: Edge vence Grid ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge | editor::SnapMode::Grid);
        editor.snap_settings().set_grid_size(1.0f);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Solver Edge");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh solver edge encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 0.45f, 0.45f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 0.95f, 0.45f, 0.0f });
        meshNode->mesh().find_or_create_edge(v0, v1);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());
        solver.register_provider(std::make_unique<editor::EdgeSnapProvider>());

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.70f, 0.46f, 0.0f });

        const editor::SnapResult result = solver.solve(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "solver encontrou resultado");
        ok &= expect(snap_mode_equals(result.mode, editor::SnapMode::Edge), "edge venceu por estar mais perto que grid");
        ok &= expect(result.target.node == meshId, "edge vencedora pertence a mesh correta");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 0.70f, 0.45f, 0.0f }, "snapped na edge vencedora");

        return ok;
    }

    bool test_solver_edge_beats_vertex()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: Edge vence Vertex ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex | editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Vertex Edge");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh vertex edge encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        meshNode->mesh().find_or_create_edge(v0, v1);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::EdgeSnapProvider>());
        solver.register_provider(std::make_unique<editor::VertexSnapProvider>());

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.05f, 0.20f, 0.0f });

        const editor::SnapResult result = solver.solve(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "solver encontrou resultado");
        ok &= expect(snap_mode_equals(result.mode, editor::SnapMode::Edge), "edge venceu por estar mais perto que vertex");
        ok &= expect(result.target.node == meshId, "edge vencedora pertence a mesh correta");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 0.05f, 0.0f, 0.0f }, "snapped no ponto mais proximo da edge");

        return ok;
    }

    bool test_solver_rejects_edge_by_distance()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: rejeita Edge por max distance ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Edge);
        editor.snap_settings().set_max_distance(0.10f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Edge Distante");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh edge distante encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 10.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 14.0f, 0.0f, 0.0f });
        meshNode->mesh().find_or_create_edge(v0, v1);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::EdgeSnapProvider>());

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f });

        const editor::SnapResult result = solver.solve(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(!result.is_valid(), "solver rejeitou edge fora do max_distance");

        return ok;
    }

} // namespace

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== Locus3D Editor Edge Snapping Smoke Test ===\n";

    bool ok = true;

    ok &= test_edge_provider_without_scene();
    ok &= test_edge_provider_empty_scene();
    ok &= test_edge_provider_mesh_without_edges();
    ok &= test_edge_provider_single_edge_midpoint();
    ok &= test_edge_provider_clamps_to_segment_start();
    ok &= test_edge_provider_clamps_to_segment_end();
    ok &= test_edge_provider_world_transform();
    ok &= test_edge_provider_chooses_nearest_edge();
    ok &= test_edge_provider_ignores_invisible_node();
    ok &= test_edge_provider_ignores_unselectable_node();
    ok &= test_solver_edge_beats_grid();
    ok &= test_solver_edge_beats_vertex();
    ok &= test_solver_rejects_edge_by_distance();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de EdgeSnapProvider passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de EdgeSnapProvider falhou.\n";
    return EXIT_FAILURE;
}