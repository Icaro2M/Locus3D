/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/selection/MeshSelection.h"
#include "editor/selection/ObjectSelection.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"

namespace locus::editor {

    /**
     * @brief Aggregates object and mesh component selection state.
     */
    class SelectionState {
    public:
        /**
         * @brief Returns mutable object selection.
         *
         * @return Mutable object selection reference.
         */
        [[nodiscard]] ObjectSelection& objects();

        /**
         * @brief Returns read-only object selection.
         *
         * @return Read-only object selection reference.
         */
        [[nodiscard]] const ObjectSelection& objects() const;

        /**
         * @brief Returns mutable mesh selection.
         *
         * @return Mutable mesh selection reference.
         */
        [[nodiscard]] MeshSelection& mesh();

        /**
         * @brief Returns read-only mesh selection.
         *
         * @return Read-only mesh selection reference.
         */
        [[nodiscard]] const MeshSelection& mesh() const;

        /**
         * @brief Returns the active selection granularity.
         *
         * @return Selection granularity.
         */
        [[nodiscard]] SelectionGranularity granularity() const;

        /**
         * @brief Changes the active selection granularity.
         *
         * @param granularity New selection granularity.
         */
        void set_granularity(SelectionGranularity granularity);

        /**
         * @brief Returns the active selection scope.
         *
         * @return Selection scope.
         */
        [[nodiscard]] SelectionScope scope() const;

        /**
         * @brief Changes the active selection scope.
         *
         * @param scope New selection scope.
         */
        void set_scope(SelectionScope scope);

        /**
         * @brief Clears both object and mesh selection state.
         */
        void clear();

        /**
         * @brief Checks whether the selection state changed since last clear_dirty.
         *
         * @return True when dirty.
         */
        [[nodiscard]] bool is_dirty() const;

        /**
         * @brief Marks the selection as dirty.
         */
        void mark_dirty();

        /**
         * @brief Clears the dirty marker.
         */
        void clear_dirty();

    private:
        ObjectSelection objects_{};
        MeshSelection mesh_{};
        SelectionGranularity granularity_ = SelectionGranularity::Object;
        SelectionScope scope_ = SelectionScope::Scene;
        bool dirty_ = true;
    };

}