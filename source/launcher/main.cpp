/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/snapping/EdgeSnapProvider.h"
#include "editor/snapping/FaceSnapProvider.h"
#include "editor/snapping/GridSnapProvider.h"
#include "editor/snapping/SnapContext.h"
#include "editor/snapping/SnapMode.h"
#include "editor/snapping/SnapResult.h"
#include "editor/snapping/SnapSolver.h"
#include "editor/snapping/SnapTarget.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
            << " | normal: ("
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
        const glm::vec3& candidate)
    {
        locus::editor::SnapContext context{};
        context.scene = &scene;
        context.originalPosition = original;
        context.candidatePosition = candidate;
        context.referenceOrigin = { 0.0f, 0.0f, 0.0f };
        return context;
    }

    bool create_quad_face(
        locus::editor::MeshNode& meshNode,
        locus::kernel::geometry::FaceHandle& outFace)
    {
        using locus::kernel::geometry::VertexHandle;

        const VertexHandle v0 = meshNode.mesh().add_vertex({ 0.0f, 0.0f, 0.0f });
        const VertexHandle v1 = meshNode.mesh().add_vertex({ 4.0f, 0.0f, 0.0f });
        const VertexHandle v2 = meshNode.mesh().add_vertex({ 4.0f, 4.0f, 0.0f });
        const VertexHandle v3 = meshNode.mesh().add_vertex({ 0.0f, 4.0f, 0.0f });

        if (!v0.is_valid() || !v1.is_valid() || !v2.is_valid() || !v3.is_valid()) {
            return false;
        }

        const std::vector<VertexHandle> vertices{ v0, v1, v2, v3 };
        outFace = meshNode.mesh().add_face(vertices);

        return outFace.is_valid();
    }

    bool test_face_provider_without_scene()
    {
        using namespace locus;

        std::cout << "\n=== FaceSnapProvider: sem scene ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Face);
        settings.set_max_distance(10.0f);

        editor::FaceSnapProvider provider;

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

    bool test_face_provider_empty_scene()
    {
        using namespace locus;

        std::cout << "\n=== FaceSnapProvider: scene vazia ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Face);
        editor.snap_settings().set_max_distance(10.0f);

        editor::FaceSnapProvider provider;

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

    bool test_face_provider_quad_inside()
    {
        using namespace locus;

        std::cout << "\n=== FaceSnapProvider: quad ponto dentro ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Face);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Quad Face");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshId.is_valid(), "mesh node criado com id valido");
        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        kernel::geometry::FaceHandle face{};
        ok &= expect(create_quad_face(*meshNode, face), "quad face criada");

        editor::FaceSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.0f, 2.0f, 3.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "face snap encontrou resultado");
        ok &= expect(snap_mode_equals(result.mode, editor::SnapMode::Face), "modo do resultado eh Face");
        ok &= expect(result.target.type == editor::SnapTargetType::Face, "target eh Face");
        ok &= expect(result.target.node == meshId, "target aponta para mesh correta");
        ok &= expect(result.target.component == 0u, "target component aponta para face 0");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 2.0f, 2.0f, 0.0f }, "projetou ponto no plano da face");

        return ok;
    }

    bool test_face_provider_quad_boundary()
    {
        using namespace locus;

        std::cout << "\n=== FaceSnapProvider: quad ponto na borda ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Face);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Quad Face Boundary");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        kernel::geometry::FaceHandle face{};
        ok &= expect(create_quad_face(*meshNode, face), "quad face criada");

        editor::FaceSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 4.0f, 2.0f, 1.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "face snap aceita ponto projetado na borda");
        ok &= expect(result.target.node == meshId, "target aponta para mesh correta");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 4.0f, 2.0f, 0.0f }, "projetou na borda da face");

        return ok;
    }

    bool test_face_provider_quad_outside()
    {
        using namespace locus;

        std::cout << "\n=== FaceSnapProvider: quad ponto fora ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Face);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Quad Face Outside");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh node encontrado");

        if (meshNode == nullptr) {
            return false;
        }

        kernel::geometry::FaceHandle face{};
        ok &= expect(create_quad_face(*meshNode, face), "quad face criada");

        editor::FaceSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 5.0f, 2.0f, 1.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(!result.is_valid(), "ponto projetado fora da face retorna invalid");

        return ok;
    }

    bool test_face_provider_world_transform()
    {
        using namespace locus;

        std::cout << "\n=== FaceSnapProvider: transform local -> mundo ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Face);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Quad Face Transformada");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh transformada encontrada");

        if (meshNode == nullptr) {
            return false;
        }

        kernel::geometry::FaceHandle face{};
        ok &= expect(create_quad_face(*meshNode, face), "quad face criada");

        meshNode->transform().set_position({ 10.0f, 5.0f, -2.0f });

        editor::FaceSnapProvider provider;

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 12.0f, 7.0f, 3.0f });

        const editor::SnapResult result = provider.snap(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "face snap com transform encontrou resultado");
        ok &= expect(result.target.node == meshId, "target aponta para mesh transformada");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 12.0f, 7.0f, -2.0f }, "snapped em coordenada de mundo");

        return ok;
    }

    bool test_solver_face_beats_grid()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: Face vence Grid ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Face | editor::SnapMode::Grid);
        editor.snap_settings().set_grid_size(1.0f);
        editor.snap_settings().set_max_distance(10.0f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Solver Face");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh solver face encontrada");

        if (meshNode == nullptr) {
            return false;
        }

        kernel::geometry::FaceHandle face{};
        ok &= expect(create_quad_face(*meshNode, face), "quad face criada");

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());
        solver.register_provider(std::make_unique<editor::FaceSnapProvider>());

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.40f, 2.40f, 0.04f });

        const editor::SnapResult result = solver.solve(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(result.is_valid(), "solver encontrou resultado");
        ok &= expect(snap_mode_equals(result.mode, editor::SnapMode::Face), "face venceu por estar mais perto que grid");
        ok &= expect(result.target.node == meshId, "face vencedora pertence a mesh correta");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 2.40f, 2.40f, 0.0f }, "snapped na face vencedora");

        return ok;
    }

    bool test_solver_rejects_face_by_distance()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: rejeita Face por max distance ===\n";

        bool ok = true;

        editor::Editor editor;
        editor.snap_settings().set_modes(editor::SnapMode::Face);
        editor.snap_settings().set_max_distance(0.10f);

        const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh Face Distante");
        editor::MeshNode* meshNode = editor.scene().find_mesh(meshId);

        ok &= expect(meshNode != nullptr, "mesh face distante encontrada");

        if (meshNode == nullptr) {
            return false;
        }

        kernel::geometry::FaceHandle face{};
        ok &= expect(create_quad_face(*meshNode, face), "quad face criada");

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::FaceSnapProvider>());

        const editor::SnapContext context = make_context(
            editor.scene(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.0f, 2.0f, 5.0f });

        const editor::SnapResult result = solver.solve(editor.snap_settings(), context);
        print_result(result);

        ok &= expect(!result.is_valid(), "solver rejeitou face fora do max_distance");

        return ok;
    }

} // namespace

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== Locus3D Editor Face Snapping Smoke Test ===\n";

    bool ok = true;

    ok &= test_face_provider_without_scene();
    ok &= test_face_provider_empty_scene();
    ok &= test_face_provider_quad_inside();
    ok &= test_face_provider_quad_boundary();
    ok &= test_face_provider_quad_outside();
    ok &= test_face_provider_world_transform();
    ok &= test_solver_face_beats_grid();
    ok &= test_solver_rejects_face_by_distance();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de FaceSnapProvider passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de FaceSnapProvider falhou.\n";
    return EXIT_FAILURE;
}