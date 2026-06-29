/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionSet.h"

namespace locus::editor {

    /**
     * @brief Stores object-level selection state for editor scene nodes.
     */
    class ObjectSelection {
    public:
        /**
         * @brief Selects a single object and makes it active.
         *
         * @param id Object identifier.
         */
        void set(SceneNodeId id);

        /**
         * @brief Replaces selected objects and chooses an active object.
         *
         * @param ids Objects to select.
         * @param active Active object. When invalid, the last selected object is used.
         */
        void set(const std::vector<SceneNodeId>& ids, SceneNodeId active = {});

        /**
         * @brief Adds an object to the selection.
         *
         * @param id Object identifier.
         * @return True when the object was added.
         */
        bool add(SceneNodeId id);

        /**
         * @brief Removes an object from the selection.
         *
         * @param id Object identifier.
         * @return True when the object was removed.
         */
        bool remove(SceneNodeId id);

        /**
         * @brief Toggles an object in the selection.
         *
         * @param id Object identifier.
         * @return True when the object is selected after the operation.
         */
        bool toggle(SceneNodeId id);

        /**
         * @brief Checks whether an object is selected.
         *
         * @param id Object identifier.
         * @return True when selected.
         */
        [[nodiscard]] bool contains(SceneNodeId id) const;

        /**
         * @brief Clears selected, active, and hovered objects.
         */
        void clear();

        /**
         * @brief Clears only the hovered object.
         */
        void clear_hovered();

        /**
         * @brief Returns selected objects.
         *
         * @return Ordered selected object list.
         */
        [[nodiscard]] const std::vector<SceneNodeId>& selected() const;

        /**
         * @brief Returns the selected object set.
         *
         * @return Read-only selection set.
         */
        [[nodiscard]] const SelectionSet<SceneNodeId>& set() const;

        /**
         * @brief Returns the active object.
         *
         * @return Active object identifier, or invalid when none is active.
         */
        [[nodiscard]] SceneNodeId active() const;

        /**
         * @brief Changes the active object.
         *
         * @param id Active object identifier.
         */
        void set_active(SceneNodeId id);

        /**
         * @brief Returns the hovered object.
         *
         * @return Hovered object identifier, or invalid when none is hovered.
         */
        [[nodiscard]] SceneNodeId hovered() const;

        /**
         * @brief Changes the hovered object.
         *
         * @param id Hovered object identifier.
         */
        void set_hovered(SceneNodeId id);

        /**
         * @brief Returns the number of selected objects.
         *
         * @return Selection count.
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Checks whether no object is selected.
         *
         * @return True when empty.
         */
        [[nodiscard]] bool empty() const;

    private:
        SelectionSet<SceneNodeId> selected_{};
        SceneNodeId active_{};
        SceneNodeId hovered_{};
    };

}