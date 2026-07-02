/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/transform/SetNodePivotCommand.h"
#include "editor/scene/NodePivot.h"
#include "editor/scene/SceneNode.h"
#include "editor/scene/SceneNodeId.h"

#include <glm/vec3.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

    using namespace locus::editor;

    int g_failures = 0;

    void expect(bool condition, std::string_view message) {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return;
        }

        std::cout << "[FAIL] " << message << '\n';
        ++g_failures;
    }

    void print_result(std::string_view label, const CommandResult& result) {
        std::cout << label << '\n';
        std::cout << "  success: " << (result.success ? "true" : "false") << '\n';
        std::cout << "  message: " << result.message << '\n';
    }

    bool nearly_equal(float a, float b, float epsilon = 0.0001f) {
        return std::abs(a - b) <= epsilon;
    }

    bool vec3_equal(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f) {
        return nearly_equal(a.x, b.x, epsilon)
            && nearly_equal(a.y, b.y, epsilon)
            && nearly_equal(a.z, b.z, epsilon);
    }

    const NodePivot& pivot_of(const Editor& editor, SceneNodeId id) {
        const SceneNode* node = editor.scene().find_node(id);
        return node->pivot();
    }

    void print_pivot(const Editor& editor, SceneNodeId id, std::string_view label) {
        const SceneNode* node = editor.scene().find_node(id);
        if (!node) {
            std::cout << label << ": <missing>\n";
            return;
        }

        const NodePivot& pivot = node->pivot();

        std::cout << label << '\n';
        std::cout << "  offset: "
            << pivot.offset.x << ", "
            << pivot.offset.y << ", "
            << pivot.offset.z << '\n';
        std::cout << "  custom: " << (pivot.custom ? "true" : "false") << '\n';
    }

    void test_set_node_pivot_command() {
        std::cout << "\n=== SetNodePivotCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId node = editor.scene().create_mesh("Pivot Target");

        expect(pivot_of(editor, node).custom == false, "pivot comeca sem custom");
        expect(vec3_equal(pivot_of(editor, node).offset, glm::vec3{ 0.0f }), "pivot comeca na origem local");

        NodePivot next{};
        next.offset = glm::vec3{ 1.0f, 2.0f, 3.0f };
        next.custom = true;

        SetNodePivotCommand command(node, next);
        CommandResult result = dispatcher.execute(command);
        print_result("set pivot", result);
        print_pivot(editor, node, "after set pivot");

        expect(result.success, "set pivot executou com sucesso");
        expect(pivot_of(editor, node).custom == true, "pivot custom foi ativado");
        expect(vec3_equal(pivot_of(editor, node).offset, glm::vec3{ 1.0f, 2.0f, 3.0f }), "pivot offset foi aplicado");

        result = dispatcher.undo(command);
        print_result("undo set pivot", result);
        print_pivot(editor, node, "after undo");

        expect(result.success, "undo do set pivot executou com sucesso");
        expect(pivot_of(editor, node).custom == false, "undo restaurou custom false");
        expect(vec3_equal(pivot_of(editor, node).offset, glm::vec3{ 0.0f }), "undo restaurou offset original");

        result = dispatcher.redo(command);
        print_result("redo set pivot", result);
        print_pivot(editor, node, "after redo");

        expect(result.success, "redo do set pivot executou com sucesso");
        expect(pivot_of(editor, node).custom == true, "redo restaurou custom true");
        expect(vec3_equal(pivot_of(editor, node).offset, glm::vec3{ 1.0f, 2.0f, 3.0f }), "redo restaurou offset aplicado");

        SetNodePivotCommand directOffset(node, glm::vec3{ -4.0f, 5.0f, -6.0f });
        result = dispatcher.execute(directOffset);
        print_result("set pivot via offset constructor", result);
        print_pivot(editor, node, "after offset constructor");

        expect(result.success, "set pivot pelo construtor com offset executou com sucesso");
        expect(pivot_of(editor, node).custom == true, "construtor com offset marcou custom true por padrao");
        expect(vec3_equal(pivot_of(editor, node).offset, glm::vec3{ -4.0f, 5.0f, -6.0f }), "construtor com offset aplicou valor");

        SetNodePivotCommand clearPivot(node, glm::vec3{ 0.0f }, false);
        result = dispatcher.execute(clearPivot);
        print_result("clear pivot via command", result);
        print_pivot(editor, node, "after clear pivot");

        expect(result.success, "clear pivot executou com sucesso");
        expect(pivot_of(editor, node).custom == false, "clear pivot desativou custom");
        expect(vec3_equal(pivot_of(editor, node).offset, glm::vec3{ 0.0f }), "clear pivot zerou offset");

        SetNodePivotCommand missing(SceneNodeId{ 999999 }, next);
        result = dispatcher.execute(missing);
        print_result("falha set pivot missing", result);

        expect(!result.success, "set pivot em no ausente falhou corretamente");

        SetNodePivotCommand invalid({}, next);
        result = dispatcher.execute(invalid);
        print_result("falha set pivot invalid", result);

        expect(!result.success, "set pivot em no invalido falhou corretamente");
    }

}

int main() {
    std::cout << "=== Locus3D Editor Pivot Command Regression Test ===\n";

    test_set_node_pivot_command();

    std::cout << "\n=== Resultado final ===\n";

    if (g_failures == 0) {
        std::cout << "[OK] todos os testes passaram\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] falhas encontradas: " << g_failures << '\n';
    return EXIT_FAILURE;
}