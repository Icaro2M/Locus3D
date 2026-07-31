/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/tools/MeshToolActivationController.h"

#include "application/ApplicationError.h"
#include "application/document/DocumentSession.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/core/ToolState.h"
#include "editor/tools/mesh/edge/BevelTool.h"
#include "editor/tools/mesh/edge/EdgeSlideTool.h"
#include "editor/tools/mesh/face/ExtrudeFaceTool.h"
#include "editor/tools/mesh/face/InsetFaceTool.h"
#include "editor/tools/mesh/face/SolidifyTool.h"
#include "editor/tools/mesh/topology/LoopCutTool.h"
#include "editor/tools/mesh/vertex/ShrinkFattenTool.h"

#include <iostream>
#include <string>

namespace locus::application {

    namespace {

        [[nodiscard]] editor::ToolContext make_tool_context(
            DocumentSession& document)
        {
            return editor::ToolContext(
                document.editor(),
                document.command_dispatcher(),
                document.history(),
                document.editor_sync().picking_sync());
        }

        [[nodiscard]] const char* granularity_name(
            editor::SelectionGranularity granularity) noexcept
        {
            switch (granularity) {
            case editor::SelectionGranularity::Object:
                return "Object";
            case editor::SelectionGranularity::Vertex:
                return "Vertex";
            case editor::SelectionGranularity::Edge:
                return "Edge";
            case editor::SelectionGranularity::Loop:
                return "Loop";
            case editor::SelectionGranularity::Face:
                return "Face";
            }

            return "Unknown";
        }

        [[nodiscard]] const char* scope_name(
            editor::SelectionScope scope) noexcept
        {
            switch (scope) {
            case editor::SelectionScope::Scene:
                return "Scene";
            case editor::SelectionScope::ActiveMesh:
                return "ActiveMesh";
            }

            return "Unknown";
        }

        [[nodiscard]] const char* tool_result_code_name(
            editor::ToolResultCode code) noexcept
        {
            switch (code) {
            case editor::ToolResultCode::Ignored:
                return "Ignored";
            case editor::ToolResultCode::Consumed:
                return "Consumed";
            case editor::ToolResultCode::Started:
                return "Started";
            case editor::ToolResultCode::Updated:
                return "Updated";
            case editor::ToolResultCode::Confirmed:
                return "Confirmed";
            case editor::ToolResultCode::Cancelled:
                return "Cancelled";
            case editor::ToolResultCode::Failed:
                return "Failed";
            }

            return "Unknown";
        }

        void print_selection_summary(
            const char* label,
            const DocumentSession& document)
        {
            const editor::SelectionState& selection =
                document.editor().selection();
            const editor::MeshSelection& meshSelection =
                selection.mesh();

            std::cout
                << "[selection] " << label
                << " scope=" << scope_name(selection.scope())
                << " granularity="
                << granularity_name(selection.granularity())
                << " activeObject="
                << selection.objects().active().value
                << " hoveredObject="
                << selection.objects().hovered().value
                << " activeMesh="
                << meshSelection.active_mesh().value
                << " vertices=" << meshSelection.vertices().size()
                << " edges=" << meshSelection.edges().size()
                << " faces=" << meshSelection.faces().size()
                << " hoverVertex="
                << meshSelection.hovered_vertex().id.value
                << " hoverEdge="
                << meshSelection.hovered_edge().id.value
                << " hoverFace="
                << meshSelection.hovered_face().id.value
                << " undo=" << document.history().undo_size()
                << " redo=" << document.history().redo_size()
                << '\n';
        }

        void print_tool_result(
            const char* label,
            const editor::ToolResult& result,
            const DocumentSession& document)
        {
            std::cout
                << "[tool] " << label
                << " code=" << tool_result_code_name(result.code)
                << " state=";

            const editor::ITool* tool =
                document.tool_manager().active_tool();

            if (tool == nullptr) {
                std::cout << "None";
            }
            else {
                switch (tool->state()) {
                case editor::ToolState::Inactive:
                    std::cout << "Inactive";
                    break;
                case editor::ToolState::Ready:
                    std::cout << "Ready";
                    break;
                case editor::ToolState::Interacting:
                    std::cout << "Interacting";
                    break;
                case editor::ToolState::Suspended:
                    std::cout << "Suspended";
                    break;
                }
            }

            if (!result.message.empty()) {
                std::cout << " message=\"" << result.message << '"';
            }

            std::cout
                << " history=("
                << document.history().undo_size()
                << '/'
                << document.history().redo_size()
                << ")\n";
        }

        [[nodiscard]] ApplicationError tool_failure_error(
            const editor::ToolResult& result)
        {
            return ApplicationError::make(
                ApplicationErrorCode::RuntimeFailure,
                result.message.empty()
                    ? "Editor tool input failed."
                    : result.message);
        }

        [[nodiscard]] ApplicationResult<void> activate_face_mesh_tool(
            DocumentSession& document,
            const editor::ToolId& id,
            const char* toolName,
            const char* actionName)
        {
            const editor::SelectionState& selection =
                document.editor().selection();

            if (selection.mesh().active_mesh().is_invalid()
                || selection.granularity()
                != editor::SelectionGranularity::Face
                || selection.mesh().faces().empty()) {
                return ApplicationError::make(
                    ApplicationErrorCode::InvalidState,
                    std::string{
                        actionName
                    } +
                    " requires an active mesh and at least one selected "
                    "face.");
            }

            editor::ToolContext toolContext = make_tool_context(document);
            const editor::ToolResult result =
                document.tool_manager().activate_tool(
                    toolContext,
                    id);

            if (result.failed()) {
                return tool_failure_error(result);
            }

            print_tool_result(
                toolName,
                result,
                document);
            print_selection_summary(
                "after mesh tool activation",
                document);

            return {};
        }

        [[nodiscard]] ApplicationResult<void> activate_edge_mesh_tool(
            DocumentSession& document,
            const editor::ToolId& id,
            const char* toolName,
            const char* actionName)
        {
            const editor::SelectionState& selection =
                document.editor().selection();

            if (selection.mesh().active_mesh().is_invalid()
                || selection.granularity()
                != editor::SelectionGranularity::Edge
                || selection.mesh().edges().empty()) {
                return ApplicationError::make(
                    ApplicationErrorCode::InvalidState,
                    std::string{
                        actionName
                    } +
                    " requires an active mesh and at least one selected "
                    "edge.");
            }

            editor::ToolContext toolContext = make_tool_context(document);
            const editor::ToolResult result =
                document.tool_manager().activate_tool(
                    toolContext,
                    id);

            if (result.failed()) {
                return tool_failure_error(result);
            }

            print_tool_result(
                toolName,
                result,
                document);
            print_selection_summary(
                "after mesh tool activation",
                document);

            return {};
        }

        [[nodiscard]] ApplicationResult<void> activate_vertex_mesh_tool(
            DocumentSession& document,
            const editor::ToolId& id,
            const char* toolName,
            const char* actionName)
        {
            const editor::SelectionState& selection =
                document.editor().selection();

            if (selection.mesh().active_mesh().is_invalid()
                || selection.granularity()
                != editor::SelectionGranularity::Vertex
                || selection.mesh().vertices().empty()) {
                return ApplicationError::make(
                    ApplicationErrorCode::InvalidState,
                    std::string{
                        actionName
                    } +
                    " requires an active mesh and at least one selected "
                    "vertex.");
            }

            editor::ToolContext toolContext = make_tool_context(document);
            const editor::ToolResult result =
                document.tool_manager().activate_tool(
                    toolContext,
                    id);

            if (result.failed()) {
                return tool_failure_error(result);
            }

            print_tool_result(
                toolName,
                result,
                document);
            print_selection_summary(
                "after mesh tool activation",
                document);

            return {};
        }

    } // namespace

    ApplicationResult<bool>
        MeshToolActivationController::activate_shortcut(
            ShortcutAction action,
            DocumentSession& document) const
    {
        switch (action) {
        case ShortcutAction::ActivateExtrudeFaceTool: {
            std::cout << "[shortcut] Extrude\n";
            const ApplicationResult<void> result =
                activate_face_mesh_tool(
                    document,
                    editor::ToolId{
                        std::string{
                            editor::ExtrudeFaceTool::Id } },
                    "ExtrudeFaceTool activation",
                    "Extrude");

            if (!result) {
                return result.error();
            }

            return true;
        }

        case ShortcutAction::ActivateInsetFaceTool: {
            std::cout << "[shortcut] Inset\n";
            const ApplicationResult<void> result =
                activate_face_mesh_tool(
                    document,
                    editor::ToolId{
                        std::string{
                            editor::InsetFaceTool::Id } },
                    "InsetFaceTool activation",
                    "Inset");

            if (!result) {
                return result.error();
            }

            return true;
        }

        case ShortcutAction::ActivateSolidifyTool: {
            std::cout << "[shortcut] Solidify\n";
            const ApplicationResult<void> result =
                activate_face_mesh_tool(
                    document,
                    editor::ToolId{
                        std::string{
                            editor::SolidifyTool::Id } },
                    "SolidifyTool activation",
                    "Solidify");

            if (!result) {
                return result.error();
            }

            return true;
        }

        case ShortcutAction::ActivateShrinkFattenTool: {
            std::cout << "[shortcut] Shrink/Fatten\n";
            const ApplicationResult<void> result =
                activate_vertex_mesh_tool(
                    document,
                    editor::ToolId{
                        std::string{
                            editor::ShrinkFattenTool::Id } },
                    "ShrinkFattenTool activation",
                    "Shrink/Fatten");

            if (!result) {
                return result.error();
            }

            return true;
        }

        case ShortcutAction::ActivateEdgeSlideTool: {
            std::cout << "[shortcut] Edge Slide\n";
            const ApplicationResult<void> result =
                activate_edge_mesh_tool(
                    document,
                    editor::ToolId{
                        std::string{
                            editor::EdgeSlideTool::Id } },
                    "EdgeSlideTool activation",
                    "Edge Slide");

            if (!result) {
                return result.error();
            }

            return true;
        }

        case ShortcutAction::ActivateBevelTool: {
            std::cout << "[shortcut] Bevel\n";
            const ApplicationResult<void> result =
                activate_edge_mesh_tool(
                    document,
                    editor::ToolId{
                        std::string{
                            editor::BevelTool::Id } },
                    "BevelTool activation",
                    "Bevel");

            if (!result) {
                return result.error();
            }

            return true;
        }

        case ShortcutAction::ActivateLoopCutTool: {
            std::cout << "[shortcut] Loop Cut\n";
            const ApplicationResult<void> result =
                activate_edge_mesh_tool(
                    document,
                    editor::ToolId{
                        std::string{
                            editor::LoopCutTool::Id } },
                    "LoopCutTool activation",
                    "Loop Cut");

            if (!result) {
                return result.error();
            }

            return true;
        }

        case ShortcutAction::None:
        case ShortcutAction::ActivateSelectTool:
        case ShortcutAction::ActivateTranslateTool:
        case ShortcutAction::ActivateRotateTool:
        case ShortcutAction::ActivateScaleTool:
        case ShortcutAction::ActivateUniversalTool:
        case ShortcutAction::SetObjectGranularity:
        case ShortcutAction::SetVertexGranularity:
        case ShortcutAction::SetEdgeGranularity:
        case ShortcutAction::SetFaceGranularity:
        case ShortcutAction::Undo:
        case ShortcutAction::Redo:
        case ShortcutAction::Save:
        case ShortcutAction::Open:
        case ShortcutAction::DeleteSelection:
        case ShortcutAction::Cancel:
            return false;
        }

        return false;
    }

    bool MeshToolActivationController::is_logged_preview_tool(
        const DocumentSession& document) const
    {
        return document.tool_manager().is_active(
                editor::ToolId{
                    std::string{
                        editor::ExtrudeFaceTool::Id } })
            || document.tool_manager().is_active(
                editor::ToolId{
                    std::string{
                        editor::InsetFaceTool::Id } })
            || document.tool_manager().is_active(
                editor::ToolId{
                    std::string{
                        editor::SolidifyTool::Id } })
            || document.tool_manager().is_active(
                editor::ToolId{
                    std::string{
                        editor::ShrinkFattenTool::Id } })
            || document.tool_manager().is_active(
                editor::ToolId{
                    std::string{
                        editor::EdgeSlideTool::Id } })
            || document.tool_manager().is_active(
                editor::ToolId{
                    std::string{
                        editor::BevelTool::Id } })
            || document.tool_manager().is_active(
                editor::ToolId{
                    std::string{
                        editor::LoopCutTool::Id } });
    }

} // namespace locus::application
