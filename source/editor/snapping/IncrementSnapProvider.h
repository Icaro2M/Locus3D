/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/snapping/ISnapProvider.h"

namespace locus::editor {

    /**
     * @brief Snaps movement deltas to a linear increment.
     */
    class IncrementSnapProvider final : public ISnapProvider {
    public:
        /**
         * @brief Returns the provider snap mode.
         *
         * @return Increment snap mode.
         */
        [[nodiscard]] SnapMode mode() const override;

        /**
         * @brief Evaluates increment snapping.
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