/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/Editor.h"
#include "editor/command/CommandResult.h"
#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/queries/SelectionHit.h"

#include "graphics/picking/PickingId.h"

#include <memory>

namespace locus::editor {

    class CommandDispatcher;
    class HistoryStack;
    class ICommand;
    class PickingSync;
    struct ToolEvent;

    /**
     * @brief Runtime services and editor access provided to active tools.
     *
     * ToolContext does not own any referenced service. The application or editor
     * runtime that creates this context must keep the supplied services alive for
     * the entire period in which the context is used.
     *
     * Platform input, GLFW state, GPU picking buffers, and viewport ownership
     * remain outside this class.
     */
    class ToolContext {
    public:
        /**
         * @brief Creates a tool context with editor access only.
         *
         * This constructor remains useful for tools and tests that do not execute
         * commands or resolve graphics picking identifiers.
         *
         * @param editor Editor facade used by tools.
         */
        explicit ToolContext(Editor& editor)
            : editor_(&editor) {
        }

        /**
         * @brief Creates a complete tool runtime context.
         *
         * @param editor Editor facade used by tools.
         * @param dispatcher Dispatcher used for command execution.
         * @param history Undo and redo history used for persistent commands.
         * @param pickingSync Mapping between graphics picking IDs and scene nodes.
         */
        ToolContext(
            Editor& editor,
            CommandDispatcher& dispatcher,
            HistoryStack& history,
            PickingSync& pickingSync)
            : editor_(&editor),
            dispatcher_(&dispatcher),
            history_(&history),
            pickingSync_(&pickingSync) {
        }

        /**
         * @brief Returns the editor facade.
         *
         * @return Mutable editor reference.
         */
        [[nodiscard]] Editor& editor() {
            return *editor_;
        }

        /**
         * @brief Returns the editor facade.
         *
         * @return Read-only editor reference.
         */
        [[nodiscard]] const Editor& editor() const {
            return *editor_;
        }

        /**
         * @brief Returns mutable editor state.
         *
         * @return Mutable state reference.
         */
        [[nodiscard]] EditorState& state() {
            return editor_->state();
        }

        /**
         * @brief Returns read-only editor state.
         *
         * @return Read-only state reference.
         */
        [[nodiscard]] const EditorState& state() const {
            return editor_->state();
        }

        /**
         * @brief Returns mutable editor scene.
         *
         * @return Mutable scene reference.
         */
        [[nodiscard]] EditorScene& scene() {
            return editor_->scene();
        }

        /**
         * @brief Returns read-only editor scene.
         *
         * @return Read-only scene reference.
         */
        [[nodiscard]] const EditorScene& scene() const {
            return editor_->scene();
        }

        /**
         * @brief Returns mutable selection state.
         *
         * @return Mutable selection state reference.
         */
        [[nodiscard]] SelectionState& selection() {
            return editor_->selection();
        }

        /**
         * @brief Returns read-only selection state.
         *
         * @return Read-only selection state reference.
         */
        [[nodiscard]] const SelectionState& selection() const {
            return editor_->selection();
        }

        /**
         * @brief Returns the high-level selection controller.
         *
         * @return Mutable selection controller reference.
         */
        [[nodiscard]] SelectionController& selection_controller() {
            return editor_->selection_controller();
        }

        /**
         * @brief Returns the high-level selection controller.
         *
         * @return Read-only selection controller reference.
         */
        [[nodiscard]] const SelectionController&
            selection_controller() const {

            return editor_->selection_controller();
        }

        /**
         * @brief Returns mutable snapping settings.
         *
         * @return Mutable snapping settings reference.
         */
        [[nodiscard]] SnapSettings& snap_settings() {
            return editor_->snap_settings();
        }

        /**
         * @brief Returns read-only snapping settings.
         *
         * @return Read-only snapping settings reference.
         */
        [[nodiscard]] const SnapSettings& snap_settings() const {
            return editor_->snap_settings();
        }

        /**
         * @brief Returns the current high-level editor mode.
         *
         * @return Current editor mode.
         */
        [[nodiscard]] EditorMode mode() const {
            return editor_->mode();
        }

        /**
         * @brief Checks whether command execution services are available.
         *
         * @return True when dispatcher and history references are configured.
         */
        [[nodiscard]] bool has_command_services() const {
            return
                dispatcher_ != nullptr &&
                history_ != nullptr;
        }

        /**
         * @brief Executes and stores an undoable editor command.
         *
         * The command is routed through HistoryStack, which delegates execution to
         * CommandDispatcher and stores the command only when appropriate.
         *
         * @param command Owned command to execute.
         * @return Command execution result.
         */
        CommandResult execute_command(
            std::unique_ptr<ICommand> command);

        /**
         * @brief Checks whether picking synchronization is available.
         *
         * @return True when a PickingSync reference is configured.
         */
        [[nodiscard]] bool has_picking_sync() const {
            return pickingSync_ != nullptr;
        }

        /**
         * @brief Resolves a graphics picking identifier into a scene node.
         *
         * @param pickingId Compact graphics picking identifier.
         * @return Associated scene node, or invalid when unavailable or unmapped.
         */
        [[nodiscard]] SceneNodeId resolve_scene_node(
            graphics::PickingId pickingId) const;

        /**
         * @brief Resolves the active mesh component under a pointer event.
         *
         * The application supplies the normalized world ray; the editor owns the
         * active mesh, granularity, node transform, and LEM handle interpretation.
         *
         * @param event Pointer event carrying a world-space ray.
         * @return Component hit with stable LEM handles, or miss.
         */
        [[nodiscard]] kernel::geometry::SelectionHit resolve_active_mesh_component(
            const ToolEvent& event) const;

        /**
         * @brief Marks editor subsystems as dirty.
         *
         * @param flags Dirty flags to add.
         */
        void mark_dirty(EditorDirtyFlags flags) {
            editor_->mark_dirty(flags);
        }

    private:
        Editor* editor_ = nullptr;
        CommandDispatcher* dispatcher_ = nullptr;
        HistoryStack* history_ = nullptr;
        PickingSync* pickingSync_ = nullptr;
    };

} // namespace locus::editor
