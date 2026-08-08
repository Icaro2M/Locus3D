/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/analyzers/thinwall/ThinWallQuality.h"

#include <glm/vec3.hpp>

namespace locus::kernel::manufacturing {

    /**
     * @brief Controls which manufacturing analyses are executed.
     *
     * All analyses are enabled by default. Process-dependent analyzers still
     * require the corresponding limits to exist in the active PrintProfile.
     */
    struct AnalysisOptions {
        // Topology.
        bool analyzeManifold = true;
        bool analyzeWatertight = true;
        bool analyzeNormalConsistency = true;
        bool analyzeOrientation = true;
        bool analyzeIslands = true;

        // Geometry.
        bool analyzeDegenerateGeometry = true;
        bool analyzeSelfIntersection = true;
        bool analyzeMinimumFeatureSize = true;
        bool analyzeVolume = true;

        // Thin wall.
        bool analyzeThinWall = true;
        ThinWallQuality thinWallQuality =
            ThinWallQuality::Balanced;

        // Process.
        bool analyzeOverhang = true;

        /**
         * @brief Direction in which the print is built.
         *
         * Defaults to positive Z. Overhang analysis normalizes the vector and
         * falls back to +Z when it is invalid.
         */
        glm::vec3 buildDirection{
            0.0f,
            0.0f,
            1.0f
        };
    };

}