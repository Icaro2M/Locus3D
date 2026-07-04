/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/scene/SceneNode.h"
#include "editor/transform/TransformSession.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace {

    constexpr float Epsilon = 0.0001f;

    bool nearly_equal(float lhs, float rhs, float epsilon = Epsilon)
    {
        return std::abs(lhs - rhs) <= epsilon;
    }

    bool nearly_equal(const glm::vec3& lhs, const glm::vec3& rhs, float epsilon = Epsilon)
    {
        return glm::length(lhs - rhs) <= epsilon;
    }

    void print_vec3(const std::string& label, const glm::vec3& value)
    {
        std::cout
            << label << ": "
            << std::fixed << std::setprecision(4)
            << value.x << ", "
            << value.y << ", "
            << value.z << '\n';
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
        const std::string& message)
    {
        if (nearly_equal(actual, expected)) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        print_vec3("  actual", actual);
        print_vec3("  expected", expected);
        return false;
    }

    locus::editor::SceneNode* require_node(
        locus::editor::Editor& editor,
        locus::editor::SceneNodeId id,
        const std::string& name,
        bool& ok)
    {
        locus::editor::SceneNode* node = editor.scene().find_node(id);

        if (!node) {
            std::cout << "[FAIL] node nao encontrado: " << name << '\n';
            ok = false;
            return nullptr;
        }

        std::cout << "[OK] node encontrado: " << name << " id=" << id.value << '\n';
        return node;
    }

    bool test_empty_selection()
    {
        using namespace locus::editor;

        std::cout << "\n=== TransformSession: selecao vazia ===\n";

        bool ok = true;

        Editor editor{};
        TransformSession session{};

        const bool began = session.begin(editor.scene(), editor.selection());

        ok &= expect(!began, "begin() com selecao vazia falhou corretamente");
        ok &= expect(!session.is_active(), "sessao nao ficou ativa com selecao vazia");
        ok &= expect(session.state() == TransformSessionState::Idle, "estado final ficou Idle");

        return ok;
    }

    bool test_world_translate_confirm()
    {
        using namespace locus::editor;

        std::cout << "\n=== TransformSession: world translate + confirm ===\n";

        bool ok = true;

        Editor editor{};

        const SceneNodeId nodeAId = editor.scene().create_empty("A");
        const SceneNodeId nodeBId = editor.scene().create_empty("B");

        SceneNode* nodeA = require_node(editor, nodeAId, "A", ok);
        SceneNode* nodeB = require_node(editor, nodeBId, "B", ok);

        if (!nodeA || !nodeB) {
            return false;
        }

        nodeA->transform().set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });
        nodeB->transform().set_position(glm::vec3{ 3.0f, 0.0f, 0.0f });

        editor.selection().objects().set(std::vector<SceneNodeId>{ nodeAId, nodeBId }, nodeAId);

        TransformSessionOptions options{};
        options.space = TransformSpace::World;
        options.pivotMode = TransformPivotMode::SelectionCenter;

        TransformSession session{};

        ok &= expect(session.begin(editor.scene(), editor.selection(), options), "begin() capturou dois targets");
        ok &= expect(session.is_active(), "sessao ficou ativa");
        ok &= expect(session.targets().size() == 2, "sessao tem dois targets");
        ok &= expect_vec3(session.pivot(), glm::vec3{ 2.0f, 0.0f, 0.0f }, "pivot da selecao ficou no centro");

        ok &= expect(session.translate(editor.scene(), glm::vec3{ 0.0f, 2.0f, 0.0f }), "translate() atualizou pelo menos um target");

        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ 1.0f, 2.0f, 0.0f }, "node A moveu em world");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 3.0f, 2.0f, 0.0f }, "node B moveu em world");
        ok &= expect(session.has_changes(), "sessao detectou alteracoes");

        ok &= expect(session.confirm(), "confirm() finalizou a sessao");
        ok &= expect(session.state() == TransformSessionState::Confirmed, "estado final ficou Confirmed");
        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ 1.0f, 2.0f, 0.0f }, "confirm manteve preview do node A");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 3.0f, 2.0f, 0.0f }, "confirm manteve preview do node B");

        return ok;
    }

    bool test_world_translate_cancel()
    {
        using namespace locus::editor;

        std::cout << "\n=== TransformSession: world translate + cancel ===\n";

        bool ok = true;

        Editor editor{};

        const SceneNodeId nodeAId = editor.scene().create_empty("A");
        const SceneNodeId nodeBId = editor.scene().create_empty("B");

        SceneNode* nodeA = require_node(editor, nodeAId, "A", ok);
        SceneNode* nodeB = require_node(editor, nodeBId, "B", ok);

        if (!nodeA || !nodeB) {
            return false;
        }

        nodeA->transform().set_position(glm::vec3{ -1.0f, 0.0f, 0.0f });
        nodeB->transform().set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });

        editor.selection().objects().set(std::vector<SceneNodeId>{ nodeAId, nodeBId }, nodeAId);

        TransformSessionOptions options{};
        options.space = TransformSpace::World;
        options.pivotMode = TransformPivotMode::SelectionCenter;

        TransformSession session{};

        ok &= expect(session.begin(editor.scene(), editor.selection(), options), "begin() iniciou");
        ok &= expect(session.translate(editor.scene(), glm::vec3{ 5.0f, 0.0f, 0.0f }), "translate() aplicou preview");

        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ 4.0f, 0.0f, 0.0f }, "node A recebeu preview");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 6.0f, 0.0f, 0.0f }, "node B recebeu preview");

        ok &= expect(session.cancel(editor.scene()), "cancel() restaurou pelo menos um target");
        ok &= expect(session.state() == TransformSessionState::Cancelled, "estado final ficou Cancelled");

        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ -1.0f, 0.0f, 0.0f }, "cancel restaurou node A");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 1.0f, 0.0f, 0.0f }, "cancel restaurou node B");

        return ok;
    }

    bool test_world_rotate_selection_center()
    {
        using namespace locus::editor;

        std::cout << "\n=== TransformSession: world rotate selection center ===\n";

        bool ok = true;

        Editor editor{};

        const SceneNodeId nodeAId = editor.scene().create_empty("A");
        const SceneNodeId nodeBId = editor.scene().create_empty("B");

        SceneNode* nodeA = require_node(editor, nodeAId, "A", ok);
        SceneNode* nodeB = require_node(editor, nodeBId, "B", ok);

        if (!nodeA || !nodeB) {
            return false;
        }

        nodeA->transform().set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });
        nodeB->transform().set_position(glm::vec3{ 3.0f, 0.0f, 0.0f });

        editor.selection().objects().set(std::vector<SceneNodeId>{ nodeAId, nodeBId }, nodeAId);

        TransformSessionOptions options{};
        options.space = TransformSpace::World;
        options.pivotMode = TransformPivotMode::SelectionCenter;

        TransformSession session{};

        const glm::quat rotation = glm::angleAxis(
            glm::half_pi<float>(),
            glm::vec3{ 0.0f, 0.0f, 1.0f });

        ok &= expect(session.begin(editor.scene(), editor.selection(), options), "begin() iniciou");
        ok &= expect_vec3(session.pivot(), glm::vec3{ 2.0f, 0.0f, 0.0f }, "pivot ficou no centro antes da rotacao");

        ok &= expect(session.rotate(editor.scene(), rotation), "rotate() aplicou preview");

        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ 2.0f, -1.0f, 0.0f }, "node A rotacionou ao redor do centro");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 2.0f, 1.0f, 0.0f }, "node B rotacionou ao redor do centro");

        ok &= expect(session.cancel(editor.scene()), "cancel() restaurou depois da rotacao");

        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ 1.0f, 0.0f, 0.0f }, "cancel restaurou node A");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 3.0f, 0.0f, 0.0f }, "cancel restaurou node B");

        return ok;
    }

    bool test_world_scale_selection_center()
    {
        using namespace locus::editor;

        std::cout << "\n=== TransformSession: world scale selection center ===\n";

        bool ok = true;

        Editor editor{};

        const SceneNodeId nodeAId = editor.scene().create_empty("A");
        const SceneNodeId nodeBId = editor.scene().create_empty("B");

        SceneNode* nodeA = require_node(editor, nodeAId, "A", ok);
        SceneNode* nodeB = require_node(editor, nodeBId, "B", ok);

        if (!nodeA || !nodeB) {
            return false;
        }

        nodeA->transform().set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });
        nodeB->transform().set_position(glm::vec3{ 3.0f, 0.0f, 0.0f });

        editor.selection().objects().set(std::vector<SceneNodeId>{ nodeAId, nodeBId }, nodeAId);

        TransformSessionOptions options{};
        options.space = TransformSpace::World;
        options.pivotMode = TransformPivotMode::SelectionCenter;

        TransformSession session{};

        ok &= expect(session.begin(editor.scene(), editor.selection(), options), "begin() iniciou");
        ok &= expect(session.scale(editor.scene(), glm::vec3{ 2.0f, 1.0f, 1.0f }), "scale() aplicou preview");

        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ 0.0f, 0.0f, 0.0f }, "node A escalou afastando do centro");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 4.0f, 0.0f, 0.0f }, "node B escalou afastando do centro");

        ok &= expect_vec3(nodeA->transform().scale(), glm::vec3{ 2.0f, 1.0f, 1.0f }, "node A recebeu escala local");
        ok &= expect_vec3(nodeB->transform().scale(), glm::vec3{ 2.0f, 1.0f, 1.0f }, "node B recebeu escala local");

        ok &= expect(session.cancel(editor.scene()), "cancel() restaurou depois da escala");

        ok &= expect_vec3(nodeA->transform().position(), glm::vec3{ 1.0f, 0.0f, 0.0f }, "cancel restaurou posicao do node A");
        ok &= expect_vec3(nodeB->transform().position(), glm::vec3{ 3.0f, 0.0f, 0.0f }, "cancel restaurou posicao do node B");
        ok &= expect_vec3(nodeA->transform().scale(), glm::vec3{ 1.0f, 1.0f, 1.0f }, "cancel restaurou escala do node A");
        ok &= expect_vec3(nodeB->transform().scale(), glm::vec3{ 1.0f, 1.0f, 1.0f }, "cancel restaurou escala do node B");

        return ok;
    }

    bool test_local_translate_with_rotation()
    {
        using namespace locus::editor;

        std::cout << "\n=== TransformSession: local translate com rotacao ===\n";

        bool ok = true;

        Editor editor{};

        const SceneNodeId nodeId = editor.scene().create_empty("Rotated");

        SceneNode* node = require_node(editor, nodeId, "Rotated", ok);

        if (!node) {
            return false;
        }

        node->transform().set_position(glm::vec3{ 0.0f, 0.0f, 0.0f });
        node->transform().set_rotation(glm::angleAxis(
            glm::half_pi<float>(),
            glm::vec3{ 0.0f, 0.0f, 1.0f }));

        editor.selection().objects().set(nodeId);

        TransformSessionOptions options{};
        options.space = TransformSpace::Local;
        options.pivotMode = TransformPivotMode::IndividualOrigins;

        TransformSession session{};

        ok &= expect(session.begin(editor.scene(), editor.selection(), options), "begin() iniciou em Local");

        ok &= expect(
            session.translate(editor.scene(), glm::vec3{ 1.0f, 0.0f, 0.0f }),
            "translate local aplicou eixo X local");

        ok &= expect_vec3(
            node->transform().position(),
            glm::vec3{ 0.0f, 1.0f, 0.0f },
            "eixo X local rotacionado moveu no Y world/local armazenado");

        ok &= expect(session.cancel(editor.scene()), "cancel() restaurou translate local");

        ok &= expect_vec3(
            node->transform().position(),
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            "cancel restaurou posicao original");

        return ok;
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor TransformSession Smoke Test ===\n";

    bool ok = true;

    ok &= test_empty_selection();
    ok &= test_world_translate_confirm();
    ok &= test_world_translate_cancel();
    ok &= test_world_rotate_selection_center();
    ok &= test_world_scale_selection_center();
    ok &= test_local_translate_with_rotation();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de TransformSession passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de TransformSession falhou.\n";
    return EXIT_FAILURE;
}