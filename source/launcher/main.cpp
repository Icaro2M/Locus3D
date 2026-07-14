/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/transform/NodeTransformChange.h"
#include "editor/command/transform/NodeTransformSnapshot.h"
#include "editor/command/transform/SetNodeTransformsCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/SceneNode.h"

#include <glm/vec3.hpp>

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

    using namespace locus::editor;

    void print_result(
        bool condition,
        const std::string& message) {

        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

    void print_command_result(
        const std::string& label,
        const CommandResult& result) {

        std::cout << label << '\n';

        std::cout
            << "  success: "
            << (result.success ? "true" : "false")
            << '\n';

        std::cout
            << "  message: "
            << result.message
            << '\n';
    }

    bool almost_equal(
        float lhs,
        float rhs,
        float epsilon = 0.0001f) {

        return std::abs(lhs - rhs) <= epsilon;
    }

    bool almost_equal(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        float epsilon = 0.0001f) {

        return
            almost_equal(lhs.x, rhs.x, epsilon) &&
            almost_equal(lhs.y, rhs.y, epsilon) &&
            almost_equal(lhs.z, rhs.z, epsilon);
    }

    NodeTransform make_transform(
        const glm::vec3& position,
        const glm::vec3& scale =
        glm::vec3{ 1.0f, 1.0f, 1.0f }) {

        NodeTransform transform{};
        transform.set_position(position);
        transform.set_scale(scale);
        return transform;
    }

    NodeTransformChange make_change(
        SceneNodeId node,
        const NodeTransform& previous,
        const NodeTransform& next) {

        NodeTransformChange change{};
        change.node = node;

        change.previous =
            NodeTransformSnapshot::capture(previous);

        change.next =
            NodeTransformSnapshot::capture(next);

        return change;
    }

    bool test_batch_execute_undo_redo() {
        std::cout
            << "\n=== SetNodeTransformsCommand: execute/undo/redo ===\n";

        Editor editor{};

        const SceneNodeId nodeA =
            editor.scene().create_empty("Node A");

        const SceneNodeId nodeB =
            editor.scene().create_empty("Node B");

        SceneNode* sceneNodeA =
            editor.scene().find_node(nodeA);

        SceneNode* sceneNodeB =
            editor.scene().find_node(nodeB);

        print_result(
            sceneNodeA != nullptr &&
            sceneNodeB != nullptr,
            "dois nodes foram criados");

        if (!sceneNodeA || !sceneNodeB) {
            return false;
        }

        const NodeTransform initialA =
            make_transform(
                glm::vec3{ 1.0f, 2.0f, 3.0f });

        const NodeTransform initialB =
            make_transform(
                glm::vec3{ -2.0f, 0.0f, 4.0f });

        const NodeTransform finalA =
            make_transform(
                glm::vec3{ 10.0f, 20.0f, 30.0f },
                glm::vec3{ 2.0f, 2.0f, 2.0f });

        const NodeTransform finalB =
            make_transform(
                glm::vec3{ -5.0f, 8.0f, 9.0f },
                glm::vec3{ 0.5f, 1.5f, 2.0f });

        sceneNodeA->transform() = initialA;
        sceneNodeB->transform() = initialB;

        std::vector<NodeTransformChange> changes{};
        changes.push_back(
            make_change(
                nodeA,
                initialA,
                finalA));

        changes.push_back(
            make_change(
                nodeB,
                initialB,
                finalB));

        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};

        const CommandResult executed =
            history.execute(
                dispatcher,
                std::make_unique<
                SetNodeTransformsCommand>(
                    changes));

        print_command_result(
            "execute batch",
            executed);

        sceneNodeA =
            editor.scene().find_node(nodeA);

        sceneNodeB =
            editor.scene().find_node(nodeB);

        const bool executeCorrect =
            executed.success &&
            sceneNodeA != nullptr &&
            sceneNodeB != nullptr &&
            almost_equal(
                sceneNodeA->transform().position(),
                finalA.position()) &&
            almost_equal(
                sceneNodeA->transform().scale(),
                finalA.scale()) &&
            almost_equal(
                sceneNodeB->transform().position(),
                finalB.position()) &&
            almost_equal(
                sceneNodeB->transform().scale(),
                finalB.scale());

        print_result(
            executeCorrect,
            "execute aplicou os transforms finais");

        print_result(
            history.undo_size() == 1u &&
            history.redo_size() == 0u,
            "dois nodes geraram uma unica entrada de historico");

        const CommandResult undone =
            history.undo(dispatcher);

        print_command_result(
            "undo batch",
            undone);

        sceneNodeA =
            editor.scene().find_node(nodeA);

        sceneNodeB =
            editor.scene().find_node(nodeB);

        const bool undoCorrect =
            undone.success &&
            sceneNodeA != nullptr &&
            sceneNodeB != nullptr &&
            almost_equal(
                sceneNodeA->transform().position(),
                initialA.position()) &&
            almost_equal(
                sceneNodeA->transform().scale(),
                initialA.scale()) &&
            almost_equal(
                sceneNodeB->transform().position(),
                initialB.position()) &&
            almost_equal(
                sceneNodeB->transform().scale(),
                initialB.scale());

        print_result(
            undoCorrect,
            "undo restaurou os transforms iniciais");

        print_result(
            history.undo_size() == 0u &&
            history.redo_size() == 1u,
            "undo moveu o batch para redo");

        const CommandResult redone =
            history.redo(dispatcher);

        print_command_result(
            "redo batch",
            redone);

        sceneNodeA =
            editor.scene().find_node(nodeA);

        sceneNodeB =
            editor.scene().find_node(nodeB);

        const bool redoCorrect =
            redone.success &&
            sceneNodeA != nullptr &&
            sceneNodeB != nullptr &&
            almost_equal(
                sceneNodeA->transform().position(),
                finalA.position()) &&
            almost_equal(
                sceneNodeB->transform().position(),
                finalB.position());

        print_result(
            redoCorrect,
            "redo reaplicou os transforms finais");

        return
            executeCorrect &&
            undoCorrect &&
            redoCorrect &&
            history.undo_size() == 1u &&
            history.redo_size() == 0u;
    }

    bool test_atomic_validation() {
        std::cout
            << "\n=== SetNodeTransformsCommand: validacao atomica ===\n";

        Editor editor{};

        const SceneNodeId validNode =
            editor.scene().create_empty("Valid Node");

        SceneNode* node =
            editor.scene().find_node(validNode);

        if (!node) {
            print_result(
                false,
                "node valido foi criado");

            return false;
        }

        const NodeTransform initial =
            make_transform(
                glm::vec3{ 2.0f, 3.0f, 4.0f });

        const NodeTransform final =
            make_transform(
                glm::vec3{ 20.0f, 30.0f, 40.0f });

        node->transform() = initial;

        SceneNodeId missingNode{};

        /*
         * Produce an identifier that is valid but does not belong to the scene.
         * Adjust this construction only if SceneNodeId uses a named factory in
         * your local version.
         */
        missingNode = SceneNodeId{
            validNode.value + 1000u
        };

        std::vector<NodeTransformChange> changes{};

        changes.push_back(
            make_change(
                validNode,
                initial,
                final));

        changes.push_back(
            make_change(
                missingNode,
                initial,
                final));

        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};

        const CommandResult result =
            history.execute(
                dispatcher,
                std::make_unique<
                SetNodeTransformsCommand>(
                    std::move(changes)));

        print_command_result(
            "execute with missing target",
            result);

        node =
            editor.scene().find_node(validNode);

        const bool preserved =
            !result.success &&
            node != nullptr &&
            almost_equal(
                node->transform().position(),
                initial.position());

        print_result(
            preserved,
            "falha nao modificou parcialmente o node valido");

        print_result(
            history.undo_size() == 0u,
            "command com falha nao entrou no historico");

        return
            preserved &&
            history.undo_size() == 0u;
    }

    bool test_empty_batch() {
        std::cout
            << "\n=== SetNodeTransformsCommand: lote vazio ===\n";

        Editor editor{};
        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};

        const CommandResult result =
            history.execute(
                dispatcher,
                std::make_unique<
                SetNodeTransformsCommand>(
                    std::vector<
                    NodeTransformChange>{}));

        print_command_result(
            "execute empty batch",
            result);

        const bool ok =
            !result.success &&
            history.undo_size() == 0u;

        print_result(
            ok,
            "lote vazio foi rejeitado sem history entry");

        return ok;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Batch Transform Command Smoke Test ===\n";

    bool ok = true;

    ok = test_batch_execute_undo_redo() && ok;
    ok = test_atomic_validation() && ok;
    ok = test_empty_batch() && ok;

    std::cout
        << "\n=== Resultado final ===\n";

    print_result(
        ok,
        "SetNodeTransformsCommand smoke test");

    return ok ? 0 : 1;
}