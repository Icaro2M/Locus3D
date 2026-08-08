/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

#include <cmath>

namespace locus::kernel::manufacturing {

    /**
     * @brief Local geometric measurement produced by thin-wall analysis.
     *
     * ThinWallResult is an algorithm-level result rather than a user-facing
     * manufacturing issue. Thin-wall analyzers may aggregate multiple local
     * results before emitting one or more PrintIssue objects.
     *
     * Source and opposite positions use the same coordinate space as the
     * analyzed LEM.
     */
    struct ThinWallResult {
        /**
         * @brief Source editable face from which the measurement originated.
         */
        geometry::FaceHandle sourceFace{};

        /**
         * @brief Opposing editable face identified by the measurement.
         *
         * Approximate methods are allowed to leave this handle invalid when
         * no specific opposing surface can be identified reliably.
         */
        geometry::FaceHandle oppositeFace{};

        /**
         * @brief Position where the local thickness measurement originated.
         */
        glm::vec3 sourcePosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Opposing surface position, when one was resolved.
         *
         * Consumers should consult has_opposite_surface() before interpreting
         * this value as an actual surface hit.
         */
        glm::vec3 oppositePosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Direction used or estimated for the thickness measurement.
         *
         * Ray-based analyzers typically store the cast direction. Approximate
         * analyzers may store the local surface-normal direction used by their
         * estimate.
         */
        glm::vec3 direction{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Estimated local wall thickness.
         *
         * Uses the coordinate units of the analyzed model.
         */
        double thickness = 0.0;

        /**
         * @brief Confidence of the local measurement in the range [0, 1].
         *
         * A value of 1 represents the strongest confidence produced by the
         * active algorithm. Confidence is relative to the algorithm and must
         * not be interpreted as mathematical certainty.
         */
        double confidence = 0.0;

        /**
         * @brief Checks whether a specific opposing editable face was found.
         *
         * @return True when oppositeFace is valid.
         */
        [[nodiscard]] bool has_opposite_surface() const noexcept
        {
            return oppositeFace.is_valid();
        }

        /**
         * @brief Checks whether the result contains a usable local thickness.
         *
         * @return True when the source face is valid and thickness is finite
         * and non-negative.
         */
        [[nodiscard]] bool is_valid() const noexcept
        {
            return
                sourceFace.is_valid() &&
                std::isfinite(thickness) &&
                thickness >= 0.0;
        }
    };

}