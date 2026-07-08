/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/gizmo/GizmoController.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/scene/NodeTransform.h"
#include "editor/transform/TransformPivotResolver.h"
#include "editor/transform/TransformSession.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

    using namespace locus::editor;

    bool nearly_equal(float a, float b, float epsilon = 0.0001f)
    {
        return std::abs(a - b) <= epsilon;
    }

    bool nearly_equal_vec3(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f)
    {
        return nearly_equal(a.x, b.x, epsilon)
            && nearly_equal(a.y, b.y, epsilon)
            && nearly_equal(a.z, b.z, epsilon);
    }

    void print_vec3(const std::string& label, const glm::vec3& value)
    {
        std::cout << label << ": "
            << value.x << ", "
            << value.y << ", "
            << value.z << '\n';
    }

    void print_result(bool condition, const std::string& message)
    {
        std::cout << (condition ? "[OK] " : "[FAIL] ") << message << '\n';
    }

    GizmoRay make_ray(const glm::vec3& origin, const glm::vec3& direction)
    {
        GizmoRay ray{};
        ray.origin = origin;
        ray.direction = direction;
        return ray;
    }

    GizmoPointerInput make_pointer(const glm::vec3& origin, const glm::vec3& direction)
    {
        GizmoPointerInput input{};
        input.ray = make_ray(origin, direction);
        input.viewDirection = glm::vec3{ 0.0f, 0.0f, -1.0f };
        input.viewRight = glm::vec3{ 1.0f, 0.0f, 0.0f };
        input.viewUp = glm::vec3{ 0.0f, 1.0f, 0.0f };
        input.visualScale = 1.0f;
        return input;
    }

    void print_controller_result(const std::string& label, const GizmoControllerResult& result)
    {
        std::cout << label << '\n';
        std::cout << "  success: " << (result.success ? "true" : "false") << '\n';
        std::cout << "  changed: " << (result.changed ? "true" : "false") << '\n';
        std::cout << "  message: " << result.message << '\n';
        std::cout << "  hit valid: " << (result.hit.is_valid() ? "true" : "false") << '\n';
        std::cout << "  hit mode: " << static_cast<int>(result.hit.mode) << '\n';
        std::cout << "  hit axis: " << static_cast<int>(result.hit.axis) << '\n';
        print_vec3("  constraint translation", result.constraint.translation);
        print_vec3("  constraint scale", result.constraint.scale);
        std::cout << "  constraint angle: " << result.constraint.angle << '\n';
    }

    bool test_controller_translate_x()
    {
        std::cout << "\n=== GizmoController: translate X em node real ===\n";

        EditorScene scene{};
        const SceneNodeId nodeId = scene.create_empty("Node Translate X");
        SceneNode* node = scene.find_node(nodeId);

        if (!node) {
            print_result(false, "node criado pode ser encontrado");
            return false;
        }

        node->transform().set_position(glm::vec3{ 0.0f, 0.0f, 0.0f });

        GizmoController controller{};

        GizmoHoverInput hover{};
        hover.mode = GizmoMode::Translate;
        hover.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        hover.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        hover.pointer = make_pointer(
            glm::vec3{ 0.75f, 0.02f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoHit hovered = controller.update_hover(hover);
        print_result(hovered.is_valid() && hovered.axis == GizmoAxis::X, "hover acertou eixo X");

        GizmoBeginDragTargetsInput begin{};
        begin.scene = &scene;
        begin.targets = std::vector<SceneNodeId>{ nodeId };
        begin.active = nodeId;
        begin.mode = GizmoMode::Translate;
        begin.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        begin.sessionOptions.space = TransformSpace::World;
        begin.sessionOptions.pivotMode = TransformPivotMode::SelectionCenter;
        begin.pointer = hover.pointer;

        const GizmoControllerResult started = controller.begin_drag(begin);
        print_controller_result("begin_drag", started);

        GizmoDragInput drag{};
        drag.pointer = make_pointer(
            glm::vec3{ 2.75f, 0.02f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoControllerResult updated = controller.update_drag(scene, drag);
        print_controller_result("update_drag", updated);

        const glm::vec3 positionAfterDrag = node->transform().position();
        print_vec3("node position after drag", positionAfterDrag);

        const bool confirmed = controller.end_drag();
        const glm::vec3 positionAfterConfirm = node->transform().position();
        print_vec3("node position after confirm", positionAfterConfirm);

        const bool ok = started.success
            && updated.success
            && updated.changed
            && confirmed
            && nearly_equal_vec3(positionAfterDrag, glm::vec3{ 2.0f, 0.0f, 0.0f }, 0.001f)
            && nearly_equal_vec3(positionAfterConfirm, glm::vec3{ 2.0f, 0.0f, 0.0f }, 0.001f);

        print_result(ok, "translate X atualizou e confirmou preview no node");
        return ok;
    }

    bool test_controller_incremental_translate_x()
    {
        std::cout << "\n=== GizmoController: translate X incremental ===\n";

        EditorScene scene{};
        const SceneNodeId nodeId = scene.create_empty("Node Incremental Translate");
        SceneNode* node = scene.find_node(nodeId);

        if (!node) {
            print_result(false, "node criado pode ser encontrado");
            return false;
        }

        GizmoController controller{};

        GizmoHoverInput hover{};
        hover.mode = GizmoMode::Translate;
        hover.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        hover.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        hover.pointer = make_pointer(
            glm::vec3{ 0.75f, 0.02f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        controller.update_hover(hover);

        GizmoBeginDragTargetsInput begin{};
        begin.scene = &scene;
        begin.targets = std::vector<SceneNodeId>{ nodeId };
        begin.active = nodeId;
        begin.mode = GizmoMode::Translate;
        begin.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        begin.sessionOptions.space = TransformSpace::World;
        begin.sessionOptions.pivotMode = TransformPivotMode::SelectionCenter;
        begin.pointer = hover.pointer;

        const GizmoControllerResult started = controller.begin_drag(begin);

        GizmoDragInput drag1{};
        drag1.pointer = make_pointer(
            glm::vec3{ 1.75f, 0.02f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoControllerResult updated1 = controller.update_drag(scene, drag1);
        const glm::vec3 positionAfterFirst = node->transform().position();

        GizmoDragInput drag2{};
        drag2.pointer = make_pointer(
            glm::vec3{ 2.75f, 0.02f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoControllerResult updated2 = controller.update_drag(scene, drag2);
        const glm::vec3 positionAfterSecond = node->transform().position();

        controller.end_drag();

        print_vec3("position after first drag", positionAfterFirst);
        print_vec3("position after second drag", positionAfterSecond);

        const bool ok = started.success
            && updated1.success
            && updated2.success
            && nearly_equal_vec3(positionAfterFirst, glm::vec3{ 1.0f, 0.0f, 0.0f }, 0.001f)
            && nearly_equal_vec3(positionAfterSecond, glm::vec3{ 2.0f, 0.0f, 0.0f }, 0.001f);

        print_result(ok, "updates incrementais nao acumulam delta absoluto duplicado");
        return ok;
    }

    bool test_controller_cancel_translate_x()
    {
        std::cout << "\n=== GizmoController: cancel restaura node ===\n";

        EditorScene scene{};
        const SceneNodeId nodeId = scene.create_empty("Node Cancel Translate");
        SceneNode* node = scene.find_node(nodeId);

        if (!node) {
            print_result(false, "node criado pode ser encontrado");
            return false;
        }

        node->transform().set_position(glm::vec3{ 10.0f, 0.0f, 0.0f });

        GizmoController controller{};

        GizmoHoverInput hover{};
        hover.mode = GizmoMode::Translate;
        hover.pivot = glm::vec3{ 10.0f, 0.0f, 0.0f };
        hover.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        hover.pointer = make_pointer(
            glm::vec3{ 10.75f, 0.02f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        controller.update_hover(hover);

        GizmoBeginDragTargetsInput begin{};
        begin.scene = &scene;
        begin.targets = std::vector<SceneNodeId>{ nodeId };
        begin.active = nodeId;
        begin.mode = GizmoMode::Translate;
        begin.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        begin.sessionOptions.space = TransformSpace::World;
        begin.sessionOptions.pivotMode = TransformPivotMode::SelectionCenter;
        begin.pointer = hover.pointer;

        const GizmoControllerResult started = controller.begin_drag(begin);

        GizmoDragInput drag{};
        drag.pointer = make_pointer(
            glm::vec3{ 12.75f, 0.02f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoControllerResult updated = controller.update_drag(scene, drag);
        const glm::vec3 positionAfterDrag = node->transform().position();

        const bool cancelled = controller.cancel_drag(scene);
        const glm::vec3 positionAfterCancel = node->transform().position();

        print_vec3("position after drag", positionAfterDrag);
        print_vec3("position after cancel", positionAfterCancel);

        const bool ok = started.success
            && updated.success
            && cancelled
            && nearly_equal_vec3(positionAfterDrag, glm::vec3{ 12.0f, 0.0f, 0.0f }, 0.001f)
            && nearly_equal_vec3(positionAfterCancel, glm::vec3{ 10.0f, 0.0f, 0.0f }, 0.001f);

        print_result(ok, "cancel_drag restaurou transform inicial");
        return ok;
    }

    bool test_controller_scale_y()
    {
        std::cout << "\n=== GizmoController: scale Y em node real ===\n";

        EditorScene scene{};
        const SceneNodeId nodeId = scene.create_empty("Node Scale Y");
        SceneNode* node = scene.find_node(nodeId);

        if (!node) {
            print_result(false, "node criado pode ser encontrado");
            return false;
        }

        GizmoController controller{};

        GizmoHoverInput hover{};
        hover.mode = GizmoMode::Scale;
        hover.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        hover.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        hover.pointer = make_pointer(
            glm::vec3{ 0.02f, 0.75f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoHit hovered = controller.update_hover(hover);
        print_result(hovered.is_valid() && hovered.axis == GizmoAxis::Y, "hover acertou eixo Y");

        GizmoBeginDragTargetsInput begin{};
        begin.scene = &scene;
        begin.targets = std::vector<SceneNodeId>{ nodeId };
        begin.active = nodeId;
        begin.mode = GizmoMode::Scale;
        begin.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        begin.sessionOptions.space = TransformSpace::World;
        begin.sessionOptions.pivotMode = TransformPivotMode::SelectionCenter;
        begin.pointer = hover.pointer;

        const GizmoControllerResult started = controller.begin_drag(begin);

        GizmoDragInput drag{};
        drag.pointer = make_pointer(
            glm::vec3{ 0.02f, 1.75f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoControllerResult updated = controller.update_drag(scene, drag);
        const glm::vec3 scaleAfterDrag = node->transform().scale();

        controller.end_drag();

        print_controller_result("update_drag", updated);
        print_vec3("node scale after drag", scaleAfterDrag);

        const bool ok = started.success
            && updated.success
            && updated.changed
            && nearly_equal_vec3(scaleAfterDrag, glm::vec3{ 1.0f, 2.0f, 1.0f }, 0.001f);

        print_result(ok, "scale Y aplicou fator esperado no node");
        return ok;
    }

    bool test_controller_rotate_z()
    {
        std::cout << "\n=== GizmoController: rotate Z em node real ===\n";

        EditorScene scene{};
        const SceneNodeId nodeId = scene.create_empty("Node Rotate Z");
        SceneNode* node = scene.find_node(nodeId);

        if (!node) {
            print_result(false, "node criado pode ser encontrado");
            return false;
        }

        GizmoController controller{};

        GizmoHoverInput hover{};
        hover.mode = GizmoMode::Rotate;
        hover.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        hover.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        hover.pointer = make_pointer(
            glm::vec3{ 1.05f, 0.0f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoHit hovered = controller.update_hover(hover);
        print_result(hovered.is_valid() && hovered.axis == GizmoAxis::Z, "hover acertou anel Z");

        GizmoBeginDragTargetsInput begin{};
        begin.scene = &scene;
        begin.targets = std::vector<SceneNodeId>{ nodeId };
        begin.active = nodeId;
        begin.mode = GizmoMode::Rotate;
        begin.orientation = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        begin.sessionOptions.space = TransformSpace::World;
        begin.sessionOptions.pivotMode = TransformPivotMode::SelectionCenter;
        begin.pointer = hover.pointer;

        const GizmoControllerResult started = controller.begin_drag(begin);

        GizmoDragInput drag{};
        drag.pointer = make_pointer(
            glm::vec3{ 0.0f, 1.05f, 5.0f },
            glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoControllerResult updated = controller.update_drag(scene, drag);
        const glm::vec3 rotatedX = node->transform().rotation() * glm::vec3{ 1.0f, 0.0f, 0.0f };

        controller.end_drag();

        print_controller_result("update_drag", updated);
        print_vec3("rotated local X", rotatedX);

        const bool ok = started.success
            && updated.success
            && updated.changed
            && nearly_equal_vec3(rotatedX, glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.001f);

        print_result(ok, "rotate Z aplicou 90 graus no node");
        return ok;
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor GizmoController Smoke Test ===\n";

    bool ok = true;

    ok = test_controller_translate_x() && ok;
    ok = test_controller_incremental_translate_x() && ok;
    ok = test_controller_cancel_translate_x() && ok;
    ok = test_controller_scale_y() && ok;
    ok = test_controller_rotate_z() && ok;

    std::cout << "\n=== Resultado final ===\n";
    print_result(ok, "GizmoController smoke test");

    return ok ? 0 : 1;
}