/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/snapping/SnapMode.h"

namespace locus::editor {

    /**
     * @brief Runtime configuration used by snapping providers and solvers.
     */
    class SnapSettings {
    public:
        /**
         * @brief Returns the enabled snapping mode mask.
         *
         * @return Enabled modes.
         */
        [[nodiscard]] SnapMode modes() const;

        /**
         * @brief Replaces the enabled snapping mode mask.
         *
         * @param modes New enabled mode mask.
         */
        void set_modes(SnapMode modes);

        /**
         * @brief Enables one snapping mode.
         *
         * @param mode Mode to enable.
         */
        void enable(SnapMode mode);

        /**
         * @brief Disables one snapping mode.
         *
         * @param mode Mode to disable.
         */
        void disable(SnapMode mode);

        /**
         * @brief Checks whether a snapping mode is enabled.
         *
         * @param mode Mode to inspect.
         * @return True when enabled.
         */
        [[nodiscard]] bool is_enabled(SnapMode mode) const;

        /**
         * @brief Returns whether the whole snapping system is enabled.
         *
         * @return True when snapping can run.
         */
        [[nodiscard]] bool snapping_enabled() const;

        /**
         * @brief Enables or disables the whole snapping system.
         *
         * @param enabled True to enable snapping.
         */
        void set_snapping_enabled(bool enabled);

        /**
         * @brief Returns the grid spacing used by grid snapping.
         *
         * @return Grid size in editor units.
         */
        [[nodiscard]] float grid_size() const;

        /**
         * @brief Changes the grid spacing.
         *
         * @param size New grid size.
         */
        void set_grid_size(float size);

        /**
         * @brief Returns the linear increment used by increment snapping.
         *
         * @return Linear increment in editor units.
         */
        [[nodiscard]] float linear_increment() const;

        /**
         * @brief Changes the linear increment.
         *
         * @param increment New linear increment.
         */
        void set_linear_increment(float increment);

        /**
         * @brief Returns the angular increment used by angle snapping.
         *
         * @return Angle increment in radians.
         */
        [[nodiscard]] float angle_increment() const;

        /**
         * @brief Changes the angular increment.
         *
         * @param radians New angle increment in radians.
         */
        void set_angle_increment(float radians);

        /**
         * @brief Returns the maximum accepted snap distance.
         *
         * @return Maximum distance in editor units.
         */
        [[nodiscard]] float max_distance() const;

        /**
         * @brief Changes the maximum accepted snap distance.
         *
         * @param distance New maximum distance.
         */
        void set_max_distance(float distance);

    private:
        SnapMode modes_ = SnapMode::Grid | SnapMode::Increment | SnapMode::Angle;
        bool snappingEnabled_ = true;
        float gridSize_ = 1.0f;
        float linearIncrement_ = 1.0f;
        float angleIncrement_ = 0.2617993878f;
        float maxDistance_ = 0.25f;
    };

} // namespace locus::editor