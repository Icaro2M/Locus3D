/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/snapping/SnapContext.h"
#include "editor/snapping/SnapMode.h"
#include "editor/snapping/SnapResult.h"
#include "editor/snapping/SnapSettings.h"

namespace locus::editor {

    /**
     * @brief Interface implemented by all snapping providers.
     */
    class ISnapProvider {
    public:
        /**
         * @brief Destroys the snap provider.
         */
        virtual ~ISnapProvider() = default;

        /**
         * @brief Returns the snap mode implemented by this provider.
         *
         * @return Provider mode.
         */
        [[nodiscard]] virtual SnapMode mode() const = 0;

        /**
         * @brief Checks whether this provider should run for the given context.
         *
         * @param settings Current snapping settings.
         * @param context Current snapping context.
         * @return True when the provider can run.
         */
        [[nodiscard]] virtual bool is_enabled(
            const SnapSettings& settings,
            const SnapContext& context) const
        {
            (void)context;
            return settings.is_enabled(mode());
        }

        /**
         * @brief Evaluates a snapping candidate.
         *
         * @param settings Current snapping settings.
         * @param context Current snapping context.
         * @return Snap result.
         */
        [[nodiscard]] virtual SnapResult snap(
            const SnapSettings& settings,
            const SnapContext& context) const = 0;
    };

} // namespace locus::editor