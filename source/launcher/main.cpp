/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/SceneNode.h"
#include "editor/sync/PickingSync.h"

#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/management/ToolManager.h"
#include "editor/tools/management/ToolRegistry.h"
#include "editor/tools/transform/TransformTool.h"

#include "editor/gizmo/GizmoAxis.h"
#include "editor/gizmo/GizmoMode.h"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

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

    const char* result_code_name(
        ToolResultCode code) {

        switch (code) {
        case ToolResultCode::Ignored:
            return "Ignored";

        case ToolResultCode::Consumed:
            return "Consumed";

        case ToolResultCode::Started:
            return "Started";

        case ToolResultCode::Updated:
            return "Updated";

        case ToolResultCode::Confirmed:
            return "Confirmed";

        case ToolResultCode::Cancelled:
            return "Cancelled";

        case ToolResultCode::Failed:
            return "Failed";
        }

        return "Unknown";
    }

    const char* tool_state_name(
        ToolState state) {

        switch (state) {
        case ToolState::Inactive:
            return "Inactive";

        case ToolState::Ready:
            return "Ready";

        case ToolState::Interacting:
            return "Interacting";

        case ToolState::Suspended:
            return "Suspended";
        }

        return "Unknown";
    }

    const char* gizmo_axis_name(
        GizmoAxis axis) {

        switch (axis) {
        case GizmoAxis::None:
            return "None";

        case GizmoAxis::X:
            return "X";

        case GizmoAxis::Y:
            return "Y";

        case GizmoAxis::Z:
            return "Z";

        case GizmoAxis::XY:
            return "XY";

        case GizmoAxis::XZ:
            return "XZ";

        case GizmoAxis::YZ:
            return "YZ";

        case GizmoAxis::XYZ:
            return "XYZ";

        case GizmoAxis::View:
            return "View";
        }

        return "Unknown";
    }

    void print_tool_result(
        const std::string& label,
        const ToolResult& result) {

        std::cout << label << '\n';

        std::cout
            << "  code: "
            << result_code_name(result.code)
            << '\n';

        std::cout
            << "  consumed: "
            << (result.was_consumed() ? "true" : "false")
            << '\n';

        std::cout
            << "  failed: "
            << (result.failed() ? "true" : "false")
            << '\n';

        std::cout
            << "  message: "
            << result.message
            << '\n';
    }

    void print_position(
        const std::string& label,
        const glm::vec3& position) {

        std::cout
            << label
            << ": "
            << position.x
            << ", "
            << position.y
            << ", "
            << position.z
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

    ToolEvent make_pointer_event(
        ToolEventType type,
        ToolPointerButton button,
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection) {

        ToolEvent event{};

        event.type = type;
        event.button = button;

        event.pointer.viewportPosition =
            glm::vec2{
                rayOrigin.x,
                rayOrigin.y
        };

        event.pointer.worldRay.origin =
            rayOrigin;

        event.pointer.worldRay.direction =
            glm::normalize(rayDirection);

        event.pointer.viewDirection =
            glm::vec3{
                0.0f,
                0.0f,
                -1.0f
        };

        event.pointer.viewRight =
            glm::vec3{
                1.0f,
                0.0f,
                0.0f
        };

        event.pointer.viewUp =
            glm::vec3{
                0.0f,
                1.0f,
                0.0f
        };

        event.pointer.visualScale = 1.0f;

        return event;
    }

    bool test_transform_tool_translation() {
        std::cout
            << "\n=== TransformTool: translate X ===\n";

        Editor editor{};
        editor.set_mode(EditorMode::Object);
        editor.clear_dirty();

        const SceneNodeId nodeId =
            editor.scene().create_empty(
                "Transform Target");

        SceneNode* node =
            editor.scene().find_node(nodeId);

        print_result(
            node != nullptr,
            "scene node foi criado");

        if (!node) {
            return false;
        }

        node->transform().set_position(
            glm::vec3{
                0.0f,
                0.0f,
                0.0f
            });

        /*
         * This test prepares the selection directly because selection command
         * behavior was already validated by the SelectTool smoke test.
         */
        editor.selection()
            .objects()
            .set(nodeId);

        print_result(
            editor.selection()
            .objects()
            .contains(nodeId) &&
            editor.selection()
            .objects()
            .active() == nodeId,
            "node ficou selecionado e ativo");

        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};
        PickingSync pickingSync{};

        ToolContext context{
            editor,
            dispatcher,
            history,
            pickingSync
        };

        ToolRegistry registry{};

        const bool registered =
            registry.register_tool(
                TransformTool::make_descriptor(),
                [] {
                    return std::make_unique<
                        TransformTool>(
                            GizmoMode::Translate);
                });

        print_result(
            registered,
            "TransformTool foi registrada");

        ToolManager manager{ registry };

        const ToolResult activation =
            manager.activate_tool(
                context,
                ToolId{ TransformTool::Id });

        print_tool_result(
            "activate TransformTool",
            activation);

        TransformTool* tool =
            dynamic_cast<TransformTool*>(
                manager.active_tool());

        print_result(
            tool != nullptr,
            "manager possui TransformTool ativa");

        print_result(
            tool != nullptr &&
            tool->state() == ToolState::Ready &&
            tool->mode() == GizmoMode::Translate,
            "TransformTool iniciou em Ready e Translate");

        if (!tool) {
            return false;
        }

        /*
         * Ray through x = 0.75, y = 0.0 and toward -Z.
         *
         * With pivot at the origin, this intersects the X handle without entering
         * the center handle.
         */
        const ToolEvent hoverEvent =
            make_pointer_event(
                ToolEventType::PointerMove,
                ToolPointerButton::None,
                glm::vec3{
                    0.75f,
                    0.0f,
                    5.0f
                },
                glm::vec3{
                    0.0f,
                    0.0f,
                    -1.0f
                });

        const ToolResult hoverResult =
            manager.handle_event(
                context,
                hoverEvent);

        print_tool_result(
            "hover X handle",
            hoverResult);

        const GizmoHit hovered =
            tool->gizmo_state().hovered;

        std::cout
            << "  hovered axis: "
            << gizmo_axis_name(hovered.axis)
            << '\n';

        print_result(
            hovered.is_valid() &&
            hovered.mode == GizmoMode::Translate &&
            hovered.axis == GizmoAxis::X,
            "ray reconheceu o eixo X do gizmo");

        print_result(
            tool->gizmo_state().pivot ==
            glm::vec3{
                0.0f,
                0.0f,
                0.0f
            },
            "pivot do gizmo ficou na origem do objeto");

        editor.clear_dirty();

        const ToolEvent pressEvent =
            make_pointer_event(
                ToolEventType::PointerPress,
                ToolPointerButton::Primary,
                glm::vec3{
                    0.75f,
                    0.0f,
                    5.0f
                },
                glm::vec3{
                    0.0f,
                    0.0f,
                    -1.0f
                });

        const ToolResult beginResult =
            manager.handle_event(
                context,
                pressEvent);

        print_tool_result(
            "begin drag X",
            beginResult);

        print_result(
            beginResult.code ==
            ToolResultCode::Started &&
            tool->state() ==
            ToolState::Interacting,
            "pointer press iniciou transform drag");

        print_result(
            tool->gizmo_state().dragging &&
            tool->gizmo_state().active.is_valid() &&
            tool->gizmo_state().active.axis ==
            GizmoAxis::X,
            "gizmo ativou handle X");

        print_result(
            tool->object_session().is_active(),
            "ObjectTransformToolSession ficou ativa");

        const glm::vec3 initialPosition =
            node->transform().position();

        print_position(
            "initial position",
            initialPosition);

        /*
         * Move the ray two world units along X.
         *
         * The closest point on the X axis changes from x = 0.75 to x = 2.75,
         * producing a translation delta of approximately +2.0.
         */
        const ToolEvent moveEvent =
            make_pointer_event(
                ToolEventType::PointerMove,
                ToolPointerButton::None,
                glm::vec3{
                    2.75f,
                    0.0f,
                    5.0f
                },
                glm::vec3{
                    0.0f,
                    0.0f,
                    -1.0f
                });

        const ToolResult updateResult =
            manager.handle_event(
                context,
                moveEvent);

        print_tool_result(
            "update drag X",
            updateResult);

        node =
            editor.scene().find_node(nodeId);

        if (!node) {
            print_result(
                false,
                "node ainda existe depois do preview");

            return false;
        }

        const glm::vec3 previewPosition =
            node->transform().position();

        print_position(
            "preview position",
            previewPosition);

        print_result(
            updateResult.code ==
            ToolResultCode::Updated,
            "pointer move atualizou preview");

        print_result(
            almost_equal(
                previewPosition,
                glm::vec3{
                    2.0f,
                    0.0f,
                    0.0f
                }),
            "preview moveu objeto em +2 no eixo X");

        print_result(
            history.undo_size() == 0u,
            "preview ainda nao entrou no historico");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Scene) &&
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Render) &&
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "preview marcou Scene, Render e Picking dirty");

        editor.clear_dirty();

        const ToolEvent releaseEvent =
            make_pointer_event(
                ToolEventType::PointerRelease,
                ToolPointerButton::Primary,
                glm::vec3{
                    2.75f,
                    0.0f,
                    5.0f
                },
                glm::vec3{
                    0.0f,
                    0.0f,
                    -1.0f
                });

        const ToolResult confirmResult =
            manager.handle_event(
                context,
                releaseEvent);

        print_tool_result(
            "release and confirm",
            confirmResult);

        node =
            editor.scene().find_node(nodeId);

        if (!node) {
            print_result(
                false,
                "node ainda existe depois do commit");

            return false;
        }

        const glm::vec3 committedPosition =
            node->transform().position();

        print_position(
            "committed position",
            committedPosition);

        print_result(
            confirmResult.code ==
            ToolResultCode::Confirmed &&
            tool->state() ==
            ToolState::Ready,
            "release confirmou e retornou a Ready");

        print_result(
            almost_equal(
                committedPosition,
                glm::vec3{
                    2.0f,
                    0.0f,
                    0.0f
                }),
            "commit preservou transform final");

        print_result(
            history.undo_size() == 1u &&
            history.redo_size() == 0u,
            "drag gerou uma unica entrada de historico");

        print_result(
            !tool->object_session().is_active() &&
            !tool->gizmo_state().dragging,
            "sessao e dragging foram encerrados");

        const CommandResult undoResult =
            history.undo(dispatcher);

        node =
            editor.scene().find_node(nodeId);

        const bool undoCorrect =
            undoResult.success &&
            node != nullptr &&
            almost_equal(
                node->transform().position(),
                glm::vec3{
                    0.0f,
                    0.0f,
                    0.0f
                });

        print_result(
            undoCorrect,
            "undo restaurou posicao inicial");

        print_result(
            history.undo_size() == 0u &&
            history.redo_size() == 1u,
            "undo moveu command para redo");

        const CommandResult redoResult =
            history.redo(dispatcher);

        node =
            editor.scene().find_node(nodeId);

        const bool redoCorrect =
            redoResult.success &&
            node != nullptr &&
            almost_equal(
                node->transform().position(),
                glm::vec3{
                    2.0f,
                    0.0f,
                    0.0f
                });

        print_result(
            redoCorrect,
            "redo reaplicou posicao final");

        print_result(
            history.undo_size() == 1u &&
            history.redo_size() == 0u,
            "redo restaurou estado do historico");

        return
            registered &&
            !activation.failed() &&
            hovered.is_valid() &&
            hovered.axis == GizmoAxis::X &&
            beginResult.code ==
            ToolResultCode::Started &&
            updateResult.code ==
            ToolResultCode::Updated &&
            almost_equal(
                previewPosition,
                glm::vec3{
                    2.0f,
                    0.0f,
                    0.0f
                }) &&
            confirmResult.code ==
            ToolResultCode::Confirmed &&
            history.undo_size() == 1u &&
            undoCorrect &&
            redoCorrect;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor TransformTool Smoke Test ===\n";

    const bool ok =
        test_transform_tool_translation();

    std::cout
        << "\n=== Resultado final ===\n";

    print_result(
        ok,
        "TransformTool object translation smoke test");

    return ok ? 0 : 1;
}