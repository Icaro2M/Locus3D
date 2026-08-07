/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/math/Bounds.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Geometric location associated with a manufacturing issue.
     *
     * Locations reference authoritative editable-mesh elements through LEM
     * handles. Optional spatial samples and bounds can provide more precise
     * information for issues that affect only part of an element.
     *
     * Derived analysis-mesh triangle indices are intentionally not stored
     * here because they may become invalid when analysis data is rebuilt.
     */
    struct IssueLocation {
        /**
         * @brief Editable-mesh vertices affected by the issue.
         */
        std::vector<geometry::VertexHandle> vertices{};

        /**
         * @brief Editable-mesh edges affected by the issue.
         */
        std::vector<geometry::EdgeHandle> edges{};

        /**
         * @brief Editable-mesh faces affected by the issue.
         */
        std::vector<geometry::FaceHandle> faces{};

        /**
         * @brief Spatial samples describing the affected region.
         *
         * Samples are expressed in the same coordinate space as the analyzed
         * mesh and are useful for localized findings such as thin walls or
         * self-intersections.
         */
        std::vector<glm::vec3> samples{};

        /**
         * @brief Bounds enclosing the affected spatial region.
         *
         * Invalid empty bounds indicate that no explicit region was provided.
         */
        math::Bounds region = math::Bounds::empty();

        /**
         * @brief Checks whether editable-mesh elements are referenced.
         *
         * @return True when at least one vertex, edge, or face is present.
         */
        [[nodiscard]] bool has_mesh_elements() const
        {
            return !vertices.empty() ||
                !edges.empty() ||
                !faces.empty();
        }

        /**
         * @brief Checks whether spatial samples are available.
         *
         * @return True when at least one sample position is present.
         */
        [[nodiscard]] bool has_samples() const
        {
            return !samples.empty();
        }

        /**
         * @brief Checks whether an explicit affected region is available.
         *
         * @return True when region contains valid bounds.
         */
        [[nodiscard]] bool has_region() const
        {
            return region.is_valid();
        }

        /**
         * @brief Checks whether this location contains no location data.
         *
         * @return True when there are no mesh references, samples, or bounds.
         */
        [[nodiscard]] bool empty() const
        {
            return !has_mesh_elements() &&
                !has_samples() &&
                !has_region();
        }

        /**
         * @brief Returns the number of referenced editable-mesh elements.
         *
         * @return Combined vertex, edge, and face reference count.
         */
        [[nodiscard]] std::size_t mesh_element_count() const
        {
            return vertices.size() +
                edges.size() +
                faces.size();
        }

        /**
         * @brief Removes all location information.
         */
        void clear()
        {
            vertices.clear();
            edges.clear();
            faces.clear();
            samples.clear();
            region.reset();
        }
    };

}