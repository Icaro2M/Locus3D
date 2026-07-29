/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/queries/SelectionHit.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"

#include <vector>

namespace locus::editor {

    /**
     * @brief Result produced by an editor selection shape query.
     */
    struct SelectionShapeResult {
        /**
         * @brief Scene objects resolved by the selection query.
         */
        std::vector<SceneNodeId> objects{};

        /**
         * @brief Mesh component resolved by the selection query.
         */
        kernel::geometry::SelectionHit component =
            kernel::geometry::SelectionHit::miss();

        /**
         * @brief Mesh node that owns the resolved component.
         */
        SceneNodeId componentNode{};

        /**
         * @brief Checks whether the query resolved at least one object.
         *
         * @return True when the result contains objects.
         */
        [[nodiscard]] bool has_objects() const {
            return !objects.empty();
        }

        /**
         * @brief Checks whether the query result is empty.
         *
         * @return True when no object was resolved.
         */
        [[nodiscard]] bool empty() const {
            return objects.empty() && !component.hit;
        }

        /**
         * @brief Clears every resolved selection target.
         */
        void clear() {
            objects.clear();
            component = kernel::geometry::SelectionHit::miss();
        }
    };

    /**
     * @brief Strategy interface for viewport selection shapes.
     *
     * Selection shapes resolve viewport interaction data into editor selection
     * targets. They do not mutate SelectionState and do not execute commands.
     */
    class ISelectionShape {
    public:
        virtual ~ISelectionShape() = default;

        ISelectionShape() = default;
        ISelectionShape(const ISelectionShape&) = delete;
        ISelectionShape& operator=(const ISelectionShape&) = delete;
        ISelectionShape(ISelectionShape&&) = default;
        ISelectionShape& operator=(ISelectionShape&&) = default;

        /**
         * @brief Resolves targets for one normalized tool event.
         *
         * @param context Tool runtime context.
         * @param event Pointer event used by the query.
         * @return Resolved editor selection targets.
         */
        [[nodiscard]]
        virtual SelectionShapeResult resolve(
            const ToolContext& context,
            const ToolEvent& event) const = 0;
    };

} // namespace locus::editor
