/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/snapping/ISnapProvider.h"

namespace locus::editor {

    /**
     * @brief Snaps candidate positions to angular increments around a reference origin.
     */
    class AngleSnapProvider final : public ISnapProvider {
    public:
        /**
         * @brief Returns the provider snap mode.
         *
         * @return Angle snap mode.
         */
        [[nodiscard]] SnapMode mode() const override;

        /**
         * @brief Evaluates angle snapping.
         *
         * The first implementation snaps around the world Z axis in the XY plane,
         * preserving the candidate Z coordinate.
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