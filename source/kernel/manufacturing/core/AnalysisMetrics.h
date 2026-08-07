/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/math/Bounds.h"

#include <cstddef>
#include <optional>

namespace locus::kernel::manufacturing {

    /**
     * @brief Non-diagnostic measurements produced during manufacturing
     * analysis.
     *
     * Metrics describe the analyzed geometry but are not themselves print
     * issues. Individual analyzers may populate only the metrics they are
     * responsible for computing.
     */
    struct AnalysisMetrics {
        /**
         * @brief Enclosed mesh volume, when it can be computed reliably.
         *
         * The value uses the cubic form of the analyzed model's coordinate
         * units.
         */
        std::optional<double> volume{};

        /**
         * @brief Total surface area, when computed.
         *
         * The value uses the squared form of the analyzed model's coordinate
         * units.
         */
        std::optional<double> surfaceArea{};

        /**
         * @brief Number of connected geometric components, when computed.
         */
        std::optional<std::size_t> connectedComponentCount{};

        /**
         * @brief Number of triangles in the derived analysis representation,
         * when available.
         */
        std::optional<std::size_t> analysisTriangleCount{};

        /**
         * @brief Bounds of the analyzed geometry.
         *
         * Invalid empty bounds indicate that this metric was not populated.
         */
        math::Bounds bounds = math::Bounds::empty();

        /**
         * @brief Checks whether a volume measurement is available.
         *
         * @return True when volume has been computed.
         */
        [[nodiscard]] bool has_volume() const
        {
            return volume.has_value();
        }

        /**
         * @brief Checks whether a surface-area measurement is available.
         *
         * @return True when surfaceArea has been computed.
         */
        [[nodiscard]] bool has_surface_area() const
        {
            return surfaceArea.has_value();
        }

        /**
         * @brief Checks whether a connected-component count is available.
         *
         * @return True when connectedComponentCount has been computed.
         */
        [[nodiscard]] bool has_connected_component_count() const
        {
            return connectedComponentCount.has_value();
        }

        /**
         * @brief Checks whether an analysis triangle count is available.
         *
         * @return True when analysisTriangleCount has been populated.
         */
        [[nodiscard]] bool has_analysis_triangle_count() const
        {
            return analysisTriangleCount.has_value();
        }

        /**
         * @brief Checks whether geometry bounds are available.
         *
         * @return True when bounds contains a valid region.
         */
        [[nodiscard]] bool has_bounds() const
        {
            return bounds.is_valid();
        }

        /**
         * @brief Restores all metrics to their unavailable state.
         */
        void clear()
        {
            volume.reset();
            surfaceArea.reset();
            connectedComponentCount.reset();
            analysisTriangleCount.reset();
            bounds.reset();
        }
    };

}