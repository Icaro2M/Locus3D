/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/sync/PickingSync.h"

#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/management/ToolManager.h"
#include "editor/tools/management/ToolRegistry.h"
#include "editor/tools/selection/SelectTool.h"

#include "graphics/picking/PickingId.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

    using namespace locus::editor;
    namespace graphics = locus::graphics;

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
            << (
                result.was_consumed()
                ? "true"
                : "false")
            << '\n';

        std::cout
            << "  failed: "
            << (
                result.failed()
                ? "true"
                : "false")
            << '\n';

        std::cout
            << "  message: "
            << result.message
            << '\n';
    }

    void print_selection(
        const Editor& editor,
        const std::string& label) {

        const ObjectSelection& objects =
            editor.selection().objects();

        std::cout << label << '\n';

        std::cout
            << "  selected: "
            << objects.size()
            << '\n';

        std::cout
            << "  active valid: "
            << (
                objects.active().is_valid()
                ? "true"
                : "false")
            << '\n';

        std::cout
            << "  hovered valid: "
            << (
                objects.hovered().is_valid()
                ? "true"
                : "false")
            << '\n';
    }

    ToolEvent make_pointer_event(
        ToolEventType type,
        graphics::PickingId pickingId,
        ToolModifiers modifiers =
        ToolModifiers::None) {

        ToolEvent event{};
        event.type = type;
        event.button =
            type == ToolEventType::PointerPress
            ? ToolPointerButton::Primary
            : ToolPointerButton::None;

        event.modifiers = modifiers;
        event.pointer.pickingId = pickingId;

        return event;
    }

    bool run_select_tool_test() {
        std::cout
            << "=== Locus3D Editor SelectTool Smoke Test ===\n";

        Editor editor{};

        const SceneNodeId nodeA =
            editor.scene().create_empty("Object A");

        const SceneNodeId nodeB =
            editor.scene().create_empty("Object B");

        const SceneNodeId nodeC =
            editor.scene().create_empty("Object C");

        print_result(
            nodeA.is_valid() &&
            nodeB.is_valid() &&
            nodeC.is_valid(),
            "tres scene nodes foram criados");

        PickingSync pickingSync{};

        const bool pickingSynchronized =
            pickingSync.sync(editor.scene());

        print_result(
            pickingSynchronized,
            "PickingSync sincronizou a cena");

        print_result(
            pickingSync.size() == 3u,
            "PickingSync criou tres mappings");

        const graphics::PickingId pickingA =
            pickingSync.picking_id(nodeA);

        const graphics::PickingId pickingB =
            pickingSync.picking_id(nodeB);

        const graphics::PickingId pickingC =
            pickingSync.picking_id(nodeC);

        print_result(
            pickingA.is_valid() &&
            pickingB.is_valid() &&
            pickingC.is_valid(),
            "todos os nodes receberam PickingId");

        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};

        ToolContext context{
            editor,
            dispatcher,
            history,
            pickingSync
        };

        ToolRegistry registry{};

        const bool registered =
            registry.register_tool(
                SelectTool::make_descriptor(),
                [] {
                    return std::make_unique<SelectTool>();
                });

        print_result(
            registered,
            "SelectTool foi registrada");

        ToolManager manager{ registry };

        const ToolResult activation =
            manager.activate_tool(
                context,
                ToolId{ SelectTool::Id });

        print_tool_result(
            "activate SelectTool",
            activation);

        print_result(
            !activation.failed() &&
            manager.is_active(
                ToolId{ SelectTool::Id }),
            "SelectTool ficou ativa");

        /*
         * Hover A.
         */
        editor.clear_dirty();

        const ToolResult hoverA =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerMove,
                    pickingA));

        print_tool_result(
            "hover Object A",
            hoverA);

        print_result(
            editor.selection()
            .objects()
            .hovered() == nodeA,
            "hover apontou para Object A");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Selection) &&
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "hover marcou Selection e Render dirty");

        /*
         * Moving over the same object should not produce another state change.
         */
        const ToolResult repeatedHover =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerMove,
                    pickingA));

        print_result(
            repeatedHover.code ==
            ToolResultCode::Ignored,
            "hover repetido foi ignorado");

        /*
         * Select A.
         */
        editor.clear_dirty();

        const ToolResult selectA =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    pickingA));

        print_tool_result(
            "select Object A",
            selectA);

        print_selection(
            editor,
            "selection after Object A");

        print_result(
            selectA.was_consumed() &&
            editor.selection()
            .objects()
            .size() == 1u &&
            editor.selection()
            .objects()
            .contains(nodeA) &&
            editor.selection()
            .objects()
            .active() == nodeA,
            "click selecionou apenas Object A");

        print_result(
            history.undo_size() == 1u &&
            history.redo_size() == 0u,
            "selecao entrou no historico");

        /*
         * Repeated exclusive click is intentionally ignored.
         */
        const ToolResult repeatedSelect =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    pickingA));

        print_result(
            repeatedSelect.code ==
            ToolResultCode::Ignored &&
            history.undo_size() == 1u,
            "selecionar novamente o mesmo objeto nao criou history entry");

        /*
         * Select B exclusively.
         */
        const ToolResult selectB =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    pickingB));

        print_tool_result(
            "select Object B",
            selectB);

        print_result(
            editor.selection()
            .objects()
            .size() == 1u &&
            editor.selection()
            .objects()
            .contains(nodeB) &&
            editor.selection()
            .objects()
            .active() == nodeB,
            "click em B substituiu selecao anterior");

        print_result(
            history.undo_size() == 2u,
            "segunda selecao entrou no historico");

        /*
         * Undo returns to A.
         */
        const CommandResult undoSelectB =
            history.undo(dispatcher);

        print_result(
            undoSelectB.success &&
            editor.selection()
            .objects()
            .size() == 1u &&
            editor.selection()
            .objects()
            .contains(nodeA),
            "undo restaurou selecao de Object A");

        print_result(
            history.undo_size() == 1u &&
            history.redo_size() == 1u,
            "undo moveu entrada para redo");

        /*
         * Redo returns to B.
         */
        const CommandResult redoSelectB =
            history.redo(dispatcher);

        print_result(
            redoSelectB.success &&
            editor.selection()
            .objects()
            .size() == 1u &&
            editor.selection()
            .objects()
            .contains(nodeB),
            "redo restaurou selecao de Object B");

        /*
         * Toggle A into the current selection.
         */
        const ToolResult toggleAOn =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    pickingA,
                    ToolModifiers::Toggle));

        print_tool_result(
            "toggle Object A on",
            toggleAOn);

        print_result(
            editor.selection()
            .objects()
            .size() == 2u &&
            editor.selection()
            .objects()
            .contains(nodeA) &&
            editor.selection()
            .objects()
            .contains(nodeB),
            "toggle adicionou Object A");

        /*
         * Toggle A out again.
         */
        const ToolResult toggleAOff =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    pickingA,
                    ToolModifiers::Toggle));

        print_tool_result(
            "toggle Object A off",
            toggleAOff);

        print_result(
            editor.selection()
            .objects()
            .size() == 1u &&
            !editor.selection()
            .objects()
            .contains(nodeA) &&
            editor.selection()
            .objects()
            .contains(nodeB),
            "segundo toggle removeu Object A");

        /*
         * Modified empty click preserves selection.
         */
        const std::size_t historyBeforeEmptyToggle =
            history.undo_size();

        const ToolResult emptyToggle =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    graphics::PickingId::invalid(),
                    ToolModifiers::Toggle));

        print_result(
            emptyToggle.code ==
            ToolResultCode::Ignored &&
            editor.selection()
            .objects()
            .contains(nodeB) &&
            history.undo_size() ==
            historyBeforeEmptyToggle,
            "toggle vazio preservou selecao e historico");

        /*
         * Empty hover clears ephemeral hover.
         */
        const ToolResult clearHover =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerMove,
                    graphics::PickingId::invalid()));

        print_tool_result(
            "clear hover",
            clearHover);

        print_result(
            !editor.selection()
            .objects()
            .hovered()
            .is_valid(),
            "pointer sem hit limpou hovered");

        /*
         * Empty unmodified click clears persistent selection.
         */
        const std::size_t historyBeforeClear =
            history.undo_size();

        const ToolResult clearSelection =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    graphics::PickingId::invalid()));

        print_tool_result(
            "clear selection",
            clearSelection);

        print_result(
            clearSelection.was_consumed() &&
            editor.selection()
            .objects()
            .empty(),
            "click vazio limpou selecao");

        print_result(
            history.undo_size() ==
            historyBeforeClear + 1u,
            "limpeza entrou no historico");

        /*
         * Undo restores B.
         */
        const CommandResult undoClear =
            history.undo(dispatcher);

        print_result(
            undoClear.success &&
            editor.selection()
            .objects()
            .size() == 1u &&
            editor.selection()
            .objects()
            .contains(nodeB),
            "undo da limpeza restaurou Object B");

        /*
         * Node C confirms a third mapping is usable.
         */
        const ToolResult selectC =
            manager.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    pickingC));

        print_result(
            selectC.was_consumed() &&
            editor.selection()
            .objects()
            .size() == 1u &&
            editor.selection()
            .objects()
            .contains(nodeC),
            "PickingId de Object C resolveu corretamente");

        const bool ok =
            pickingSynchronized &&
            registered &&
            !activation.failed() &&
            editor.selection()
            .objects()
            .contains(nodeC) &&
            history.can_undo();

        std::cout
            << "\n=== Resultado final ===\n";

        print_result(
            ok,
            "SelectTool point selection smoke test");

        return ok;
    }

} // namespace

int main() {
    return run_select_tool_test()
        ? 0
        : 1;
}