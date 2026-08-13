/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/EditorContext.h"
#include "editor/EditorState.h"
#include "editor/selection/SelectionController.h"
#include "editor/snapping/SnapSettings.h"

#include <memory>

namespace locus::editor {

    /**
     * @brief Facade for high-level editor state and scene operations.
     */
    class Editor {
    public:
        /**
         * @brief Creates an editor facade with default context.
         */
        Editor();

        /**
         * @brief Creates an editor facade using an external context.
         *
         * @param context Editor service context.
         */
        explicit Editor(EditorContext context);

        /**
         * @brief Destroys the editor facade.
         */
        ~Editor();

        Editor(const Editor&) = delete;
        Editor& operator=(const Editor&) = delete;
        Editor(Editor&&) = delete;
        Editor& operator=(Editor&&) = delete;

        /**
         * @brief Returns mutable editor state.
         *
         * @return Mutable state reference.
         */
        [[nodiscard]] EditorState& state();

        /**
         * @brief Returns read-only editor state.
         *
         * @return Read-only state reference.
         */
        [[nodiscard]] const EditorState& state() const;

        /**
         * @brief Returns mutable editor context.
         *
         * @return Mutable context reference.
         */
        [[nodiscard]] EditorContext& context();

        /**
         * @brief Returns read-only editor context.
         *
         * @return Read-only context reference.
         */
        [[nodiscard]] const EditorContext& context() const;

        /**
         * @brief Returns mutable access to the editor scene.
         *
         * @return Mutable scene reference.
         */
        [[nodiscard]] EditorScene& scene();

        /**
         * @brief Returns read-only access to the editor scene.
         *
         * @return Read-only scene reference.
         */
        [[nodiscard]] const EditorScene& scene() const;

        /**
         * @brief Replaces the scene after a fully validated document load.
         *
         * @param scene New scene ownership.
         */
        void replace_scene(EditorScene scene);

        /**
         * @brief Returns mutable access to the selection state.
         *
         * @return Mutable selection state reference.
         */
        [[nodiscard]] SelectionState& selection();

        /**
         * @brief Returns read-only access to the selection state.
         *
         * @return Read-only selection state reference.
         */
        [[nodiscard]] const SelectionState& selection() const;

        /**
         * @brief Returns mutable access to snapping settings.
         *
         * @return Mutable snapping settings reference.
         */
        [[nodiscard]] SnapSettings& snap_settings();

        /**
         * @brief Returns read-only access to snapping settings.
         *
         * @return Read-only snapping settings reference.
         */
        [[nodiscard]] const SnapSettings& snap_settings() const;

        /**
         * @brief Returns the selection controller.
         *
         * @return Selection controller reference.
         */
        [[nodiscard]] SelectionController& selection_controller();

        /**
         * @brief Returns the selection controller.
         *
         * @return Read-only selection controller reference.
         */
        [[nodiscard]] const SelectionController& selection_controller() const;

        /**
         * @brief Changes the current high-level editor mode.
         *
         * @param mode New editor mode.
         */
        void set_mode(EditorMode mode);

        /**
         * @brief Returns the current high-level editor mode.
         *
         * @return Current editor mode.
         */
        [[nodiscard]] EditorMode mode() const;

        /**
         * @brief Marks editor subsystems as dirty.
         *
         * @param flags Dirty flags to add.
         */
        void mark_dirty(EditorDirtyFlags flags);

        /**
         * @brief Clears editor dirty flags.
         *
         * @param flags Dirty flags to clear.
         */
        void clear_dirty(EditorDirtyFlags flags = EditorDirtyFlags::All);

        /**
         * @brief Returns the editor dirty flag mask.
         *
         * @return Dirty flags.
         */
        [[nodiscard]] EditorDirtyFlags dirty_flags() const;

    private:
        void rebuild_controllers();

        EditorContext context_{};
        EditorState state_{};
        std::unique_ptr<SelectionController> selectionController_{};
    };

} // namespace locus::editor
