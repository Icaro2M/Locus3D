/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/snapping/GridSnapProvider.h"
#include "editor/snapping/SnapContext.h"
#include "editor/snapping/SnapMode.h"
#include "editor/snapping/SnapResult.h"
#include "editor/snapping/SnapSolver.h"
#include "editor/snapping/SnapTarget.h"
#include "editor/snapping/VertexSnapProvider.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstdlib>
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

    bool test_vertex_provider_without_scene()
    {
        using namespace locus;

        std::cout << "\n=== VertexSnapProvider: sem scene ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Vertex);
        settings.set_max_distance(10.0f);

        editor::VertexSnapProvider provider;

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

    bool test_vertex_provider_empty_scene()
    {
        using namespace locus;

        std::cout << "\n=== VertexSnapProvider: scene vazia ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex);
        editor.snap_settings().set_max_distance(10.0f);

        editor::VertexSnapProvider provider;

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

    bool test_vertex_provider_single_mesh_local_vertices()
    {
        using namespace locus;

        std::cout << "\n=== VertexSnapProvider: mesh simples local ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh A");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshId.is_valid(), "mesh node criado com id valido");
        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        const kernel::geometry::VertexHandle v0 = meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v1 = meshNode->mesh().add_vertex({ 2.0f, 0.0f, 0.0f });
        const kernel::geometry::VertexHandle v2 = meshNode->mesh().add_vertex({ 0.0f, 3.0f, 0.0f });

        ok &= expect(v0.is_valid(), "v0 criado");
        ok &= expect(v1.is_valid(), "v1 criado");
        ok &= expect(v2.is_valid(), "v2 criado");

        editor::VertexSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.80f, 0.15f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "vertex snap encontrou resultado");
        ok &= expect(result.mode == editor::SnapMode::Vertex, "modo do resultado eh Vertex");
        ok &= expect(result.target.type == editor::SnapTargetType::Vertex, "target eh Vertex");
        ok &= expect(result.target.node == meshId, "target aponta para mesh correta");
        ok &= expect(result.target.component == 1u, "target component aponta para v1");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 2.0f, 0.0f, 0.0f }, "snapped no vertice local mais proximo");

        return ok;
    }

    bool test_vertex_provider_world_transform()
    {
        using namespace locus;

        std::cout << "\n=== VertexSnapProvider: transform local -> mundo ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Transformada");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh transformada encontrada");

        if (meshNode == nullptr) {
            return false;
        }

        meshNode->mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        meshNode->mesh().add_vertex({ 1.0f, 0.0f, 0.0f });
        meshNode->transform().set_position({ 10.0f, 5.0f, -2.0f });

        editor::VertexSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 10.90f, 5.10f, -2.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "vertex snap com transform encontrou resultado");
        ok &= expect(result.target.node == meshId, "target aponta para mesh transformada");
        ok &= expect(result.target.component == 1u, "target component aponta para vertice local 1");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 11.0f, 5.0f, -2.0f }, "snapped em coordenada de mundo");

        return ok;
    }

    bool test_vertex_provider_ignores_invisible_node()
    {
        using namespace locus;

        std::cout << "\n=== VertexSnapProvider: ignora node invisivel ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId visibleId = editor.scene().create_mesh("Visible Mesh");
        const editor::SceneNodeId invisibleId = editor.scene().create_mesh("Invisible Mesh");

        editor::MeshNode* visibleNode = editor.scene().find_mesh(visibleId);
        editor::MeshNode* invisibleNode = editor.scene().find_mesh(invisibleId);

        ok &= expect(visibleNode != nullptr, "visible node encontrado");
        ok &= expect(invisibleNode != nullptr, "invisible node encontrado");

        if (visibleNode == nullptr || invisibleNode == nullptr) {
            return false;
        }

        visibleNode->mesh().add_vertex({ 5.0f, 0.0f, 0.0f });
        invisibleNode->mesh().add_vertex({ 1.0f, 0.0f, 0.0f });
        invisibleNode->metadata().visible = false;

        editor::VertexSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.05f, 0.0f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "snap encontrou apenas node visivel");
        ok &= expect(result.target.node == visibleId, "node invisivel foi ignorado");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 5.0f, 0.0f, 0.0f }, "snapped no vertice visivel restante");

        return ok;
    }

    bool test_vertex_provider_ignores_unselectable_node()
    {
        using namespace locus;

        std::cout << "\n=== VertexSnapProvider: ignora node nao selecionavel ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId selectableId = editor.scene().create_mesh("Selectable Mesh");
        const editor::SceneNodeId unselectableId = editor.scene().create_mesh("Unselectable Mesh");

        editor::MeshNode* selectableNode = editor.scene().find_mesh(selectableId);
        editor::MeshNode* unselectableNode = editor.scene().find_mesh(unselectableId);

        ok &= expect(selectableNode != nullptr, "selectable node encontrado");
        ok &= expect(unselectableNode != nullptr, "unselectable node encontrado");

        if (selectableNode == nullptr || unselectableNode == nullptr) {
            return false;
        }

        selectableNode->mesh().add_vertex({ 5.0f, 0.0f, 0.0f });
        unselectableNode->mesh().add_vertex({ 1.0f, 0.0f, 0.0f });
        unselectableNode->metadata().selectable = false;

        editor::VertexSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.05f, 0.0f, 0.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "snap encontrou apenas node selecionavel");
        ok &= expect(result.target.node == selectableId, "node nao selecionavel foi ignorado");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 5.0f, 0.0f, 0.0f }, "snapped no vertice selecionavel restante");

        return ok;
    }

    bool test_solver_with_vertex_and_grid()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: Vertex vence Grid ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex | editor::SnapMode::Grid);
        editor.snap_settings().set_grid_size(1.0f);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Solver");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh solver encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        meshNode->mesh().add_vertex({ 0.45f, 0.45f, 0.0f });

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());
        solver.register_provider(std::make_unique<editor::VertexSnapProvider>());

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.46f, 0.46f, 0.0f });

        const editor::SnapResult result = solver.solve(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "solver encontrou resultado");
        ok &= expect(result.mode == editor::SnapMode::Vertex, "vertex venceu por estar mais perto que grid");
        ok &= expect(result.target.node == meshId, "vertex vencedor pertence a mesh correta");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 0.45f, 0.45f, 0.0f }, "snapped no vertex vencedor");

        return ok;
    }

    bool test_solver_rejects_vertex_by_distance()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: rejeita Vertex por max distance ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Vertex);
        editor.snap_settings().set_max_distance(0.10f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Distante");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh distante encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        meshNode->mesh().add_vertex({ 10.0f, 0.0f, 0.0f });

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::VertexSnapProvider>());

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f });

        const editor::SnapResult result = solver.solve(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(!result.is_valid(), "solver rejeitou vertex fora do max_distance");

        return ok;
    }

} // namespace

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== Locus3D Editor Vertex Snapping Smoke Test ===\n";

    bool ok = true;

    ok &= test_vertex_provider_without_scene();
    ok &= test_vertex_provider_empty_scene();
    ok &= test_vertex_provider_single_mesh_local_vertices();
    ok &= test_vertex_provider_world_transform();
    ok &= test_vertex_provider_ignores_invisible_node();
    ok &= test_vertex_provider_ignores_unselectable_node();
    ok &= test_solver_with_vertex_and_grid();
    ok &= test_solver_rejects_vertex_by_distance();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de VertexSnapProvider passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de VertexSnapProvider falhou.\n";
    return EXIT_FAILURE;
}