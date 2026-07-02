/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/transform/RotateNodeCommand.h"
#include "editor/command/transform/ScaleNodeCommand.h"
#include "editor/command/transform/SetNodeTransformCommand.h"
#include "editor/command/transform/TranslateNodeCommand.h"
#include "editor/scene/NodeTransform.h"
#include "editor/scene/SceneNode.h"
#include "editor/scene/SceneNodeId.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/geometric.hpp>
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

    bool quat_equal(const glm::quat& a, const glm::quat& b, float epsilon = 0.0001f) {
        return nearly_equal(a.w, b.w, epsilon)
            && nearly_equal(a.x, b.x, epsilon)
            && nearly_equal(a.y, b.y, epsilon)
            && nearly_equal(a.z, b.z, epsilon);
    }

    const NodeTransform& transform_of(const Editor& editor, SceneNodeId id) {
        const SceneNode* node = editor.scene().find_node(id);
        return node->transform();
    }

    void print_transform(const Editor& editor, SceneNodeId id, std::string_view label) {
        const SceneNode* node = editor.scene().find_node(id);
        if (!node) {
            std::cout << label << ": <missing>\n";
            return;
        }

        const NodeTransform& transform = node->transform();
        const glm::vec3 position = transform.position();
        const glm::quat rotation = transform.rotation();
        const glm::vec3 scale = transform.scale();

        std::cout << label << '\n';
        std::cout << "  position: "
            << position.x << ", "
            << position.y << ", "
            << position.z << '\n';
        std::cout << "  rotation: "
            << rotation.w << ", "
            << rotation.x << ", "
            << rotation.y << ", "
            << rotation.z << '\n';
        std::cout << "  scale: "
            << scale.x << ", "
            << scale.y << ", "
            << scale.z << '\n';
    }

    void test_set_node_transform_command() {
        std::cout << "\n=== SetNodeTransformCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId node = editor.scene().create_empty("Transform Target");

        NodeTransform next{};
        next.set_position(glm::vec3{ 3.0f, 4.0f, 5.0f });
        next.set_rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3{ 0.0f, 1.0f, 0.0f }));
        next.set_scale(glm::vec3{ 2.0f, 3.0f, 4.0f });

        SetNodeTransformCommand command(node, next);
        CommandResult result = dispatcher.execute(command);
        print_result("set transform", result);
        print_transform(editor, node, "after set");

        expect(result.success, "set transform executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 3.0f, 4.0f, 5.0f }), "position foi aplicada");
        expect(quat_equal(transform_of(editor, node).rotation(), next.rotation()), "rotation foi aplicada");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 2.0f, 3.0f, 4.0f }), "scale foi aplicada");

        result = dispatcher.undo(command);
        print_result("undo set transform", result);
        print_transform(editor, node, "after undo");

        expect(result.success, "undo do set transform executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 0.0f }), "position voltou ao default");
        expect(quat_equal(transform_of(editor, node).rotation(), glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f }), "rotation voltou ao default");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 1.0f }), "scale voltou ao default");

        result = dispatcher.redo(command);
        print_result("redo set transform", result);
        print_transform(editor, node, "after redo");

        expect(result.success, "redo do set transform executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 3.0f, 4.0f, 5.0f }), "redo reaplicou position");
        expect(quat_equal(transform_of(editor, node).rotation(), next.rotation()), "redo reaplicou rotation");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 2.0f, 3.0f, 4.0f }), "redo reaplicou scale");

        SetNodeTransformCommand missing(SceneNodeId{ 999999 }, next);
        result = dispatcher.execute(missing);
        print_result("falha set transform missing", result);

        expect(!result.success, "set transform em no ausente falhou corretamente");

        SetNodeTransformCommand invalid({}, next);
        result = dispatcher.execute(invalid);
        print_result("falha set transform invalid", result);

        expect(!result.success, "set transform em no invalido falhou corretamente");
    }

    void test_translate_node_command() {
        std::cout << "\n=== TranslateNodeCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId node = editor.scene().create_mesh("Translate Target");

        NodeTransform initial{};
        initial.set_position(glm::vec3{ 1.0f, 2.0f, 3.0f });
        editor.scene().find_node(node)->transform() = initial;

        TranslateNodeCommand command(node, glm::vec3{ 10.0f, -2.0f, 5.0f });
        CommandResult result = dispatcher.execute(command);
        print_result("translate node", result);
        print_transform(editor, node, "after translate");

        expect(result.success, "translate executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 11.0f, 0.0f, 8.0f }), "translate somou delta na position");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 1.0f }), "translate nao alterou scale");

        result = dispatcher.undo(command);
        print_result("undo translate", result);

        expect(result.success, "undo do translate executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 1.0f, 2.0f, 3.0f }), "undo restaurou position inicial");

        result = dispatcher.redo(command);
        print_result("redo translate", result);

        expect(result.success, "redo do translate executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 11.0f, 0.0f, 8.0f }), "redo reaplicou translation");

        TranslateNodeCommand missing(SceneNodeId{ 999999 }, glm::vec3{ 1.0f });
        result = dispatcher.execute(missing);
        print_result("falha translate missing", result);

        expect(!result.success, "translate em no ausente falhou corretamente");
    }

    void test_rotate_node_command() {
        std::cout << "\n=== RotateNodeCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId node = editor.scene().create_empty("Rotate Target");

        const glm::quat initialRotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3{ 1.0f, 0.0f, 0.0f });

        NodeTransform initial{};
        initial.set_rotation(initialRotation);
        editor.scene().find_node(node)->transform() = initial;

        const glm::quat delta = glm::angleAxis(glm::half_pi<float>(), glm::vec3{ 0.0f, 1.0f, 0.0f });
        const glm::quat expected = glm::normalize(delta * initialRotation);

        RotateNodeCommand command(node, delta);
        CommandResult result = dispatcher.execute(command);
        print_result("rotate node", result);
        print_transform(editor, node, "after rotate");

        expect(result.success, "rotate executou com sucesso");
        expect(quat_equal(transform_of(editor, node).rotation(), expected), "rotate multiplicou delta pela rotacao atual");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 0.0f }), "rotate nao alterou position");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 1.0f }), "rotate nao alterou scale");

        result = dispatcher.undo(command);
        print_result("undo rotate", result);

        expect(result.success, "undo do rotate executou com sucesso");
        expect(quat_equal(transform_of(editor, node).rotation(), initialRotation), "undo restaurou rotation inicial");

        result = dispatcher.redo(command);
        print_result("redo rotate", result);

        expect(result.success, "redo do rotate executou com sucesso");
        expect(quat_equal(transform_of(editor, node).rotation(), expected), "redo reaplicou rotation");

        RotateNodeCommand missing(SceneNodeId{ 999999 }, delta);
        result = dispatcher.execute(missing);
        print_result("falha rotate missing", result);

        expect(!result.success, "rotate em no ausente falhou corretamente");
    }

    void test_scale_node_command() {
        std::cout << "\n=== ScaleNodeCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId node = editor.scene().create_mesh("Scale Target");

        NodeTransform initial{};
        initial.set_position(glm::vec3{ 7.0f, 8.0f, 9.0f });
        initial.set_scale(glm::vec3{ 2.0f, 3.0f, 4.0f });
        editor.scene().find_node(node)->transform() = initial;

        ScaleNodeCommand command(node, glm::vec3{ 0.5f, 2.0f, 3.0f });
        CommandResult result = dispatcher.execute(command);
        print_result("scale node", result);
        print_transform(editor, node, "after scale");

        expect(result.success, "scale executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 1.0f, 6.0f, 12.0f }), "scale multiplicou fator por eixo");
        expect(vec3_equal(transform_of(editor, node).position(), glm::vec3{ 7.0f, 8.0f, 9.0f }), "scale nao alterou position");

        result = dispatcher.undo(command);
        print_result("undo scale", result);

        expect(result.success, "undo do scale executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 2.0f, 3.0f, 4.0f }), "undo restaurou scale inicial");

        result = dispatcher.redo(command);
        print_result("redo scale", result);

        expect(result.success, "redo do scale executou com sucesso");
        expect(vec3_equal(transform_of(editor, node).scale(), glm::vec3{ 1.0f, 6.0f, 12.0f }), "redo reaplicou scale");

        ScaleNodeCommand missing(SceneNodeId{ 999999 }, glm::vec3{ 2.0f });
        result = dispatcher.execute(missing);
        print_result("falha scale missing", result);

        expect(!result.success, "scale em no ausente falhou corretamente");
    }

}

int main() {
    std::cout << "=== Locus3D Editor Transform Commands Regression Test ===\n";

    test_set_node_transform_command();
    test_translate_node_command();
    test_rotate_node_command();
    test_scale_node_command();

    std::cout << "\n=== Resultado final ===\n";

    if (g_failures == 0) {
        std::cout << "[OK] todos os testes passaram\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] falhas encontradas: " << g_failures << '\n';
    return EXIT_FAILURE;
}