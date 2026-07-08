/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/snapping/ISnapProvider.h"

namespace locus::editor {

    /**
     * @brief Snaps positions to the nearest visible mesh vertex in the editor scene.
     */
    class VertexSnapProvider final : public ISnapProvider {
    public:
        /**
         * @brief Returns the provider snap mode.
         *
         * @return Vertex snap mode.
         */
        [[nodiscard]] SnapMode mode() const override;

        /**
         * @brief Checks whether vertex snapping can run for the given context.
         *
         * @param settings Current snapping settings.
         * @param context Current snapping context.
         * @return True when vertex snapping is enabled and a scene is available.
         */
        [[nodiscard]] bool is_enabled(
            const SnapSettings& settings,
            const SnapContext& context) const override;

        /**
         * @brief Evaluates vertex snapping.
         *
         * @param settings Current snapping settings.
         * @param context Current snapping context.
         * @return Snap result.
         */
        [[nodiscard]] SnapResult snap(
            const SnapSettings& settings,
            const SnapContext& context) const override;
    };

} // namespace locus::editor