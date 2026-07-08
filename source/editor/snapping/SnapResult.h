/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/snapping/SnapMode.h"
#include "editor/snapping/SnapTarget.h"

#include <glm/glm.hpp>

namespace locus::editor {

    /**
     * @brief Result produced by one snap provider.
     */
    struct SnapResult {
        /**
         * @brief Whether the result contains a valid snapped position.
         */
        bool valid = false;

        /**
         * @brief Snapping mode that produced the result.
         */
        SnapMode mode = SnapMode::None;

        /**
         * @brief Target element selected by the snap provider.
         */
        SnapTarget target{};

        /**
         * @brief Original input position before snapping.
         */
        glm::vec3 originalPosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Candidate position evaluated by the provider.
         */
        glm::vec3 candidatePosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Final snapped position.
         */
        glm::vec3 snappedPosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Distance from candidatePosition to snappedPosition.
         */
        float distance = 0.0f;

        /**
         * @brief Score used by SnapSolver to compare results.
         */
        float score = 0.0f;

        /**
         * @brief Creates an invalid snap result.
         *
         * @return Invalid result.
         */
        [[nodiscard]] static SnapResult none()
        {
            return {};
        }

        /**
         * @brief Creates a valid snap result.
         *
         * @param mode Mode that produced the snap.
         * @param target Target element.
         * @param original Original position.
         * @param candidate Candidate position.
         * @param snapped Snapped position.
         * @param distance Snap distance.
         * @param score Sorting score.
         * @return Valid snap result.
         */
        [[nodiscard]] static SnapResult make(
            SnapMode mode,
            const SnapTarget& target,
            const glm::vec3& original,
            const glm::vec3& candidate,
            const glm::vec3& snapped,
            float distance,
            float score)
        {
            SnapResult result{};
            result.valid = true;
            result.mode = mode;
            result.target = target;
            result.originalPosition = original;
            result.candidatePosition = candidate;
            result.snappedPosition = snapped;
            result.distance = distance;
            result.score = score;
            return result;
        }

        /**
         * @brief Checks whether the result is valid.
         *
         * @return True when valid.
         */
        [[nodiscard]] bool is_valid() const
        {
            return valid;
        }
    };

} // namespace locus::editor