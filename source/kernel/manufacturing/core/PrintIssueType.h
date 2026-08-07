/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::manufacturing {

    /**
     * @brief Semantic category of a manufacturing analysis issue.
     *
     * Issue types describe manufacturing-relevant conditions discovered by
     * analyzers. They are independent from presentation so editor and UI code
     * can decide how each issue should be displayed.
     */
    enum class PrintIssueType {
        /**
         * @brief Mesh contains an edge that does not participate in any face.
         */
        LooseEdge,

        /**
         * @brief Mesh contains an edge belonging to an open surface boundary.
         */
        OpenBoundary,

        /**
         * @brief Mesh contains an edge shared by a non-manifold set of faces.
         */
        NonManifoldEdge,

        /**
         * @brief Adjacent faces have inconsistent winding or normal orientation.
         */
        InconsistentNormals,

        /**
         * @brief A closed component is consistently oriented inward.
         */
        InvertedOrientation,

        /**
         * @brief Mesh contains a disconnected component considered an island.
         */
        DisconnectedIsland,

        /**
         * @brief Mesh contains geometrically degenerate surface elements.
         */
        DegenerateGeometry,

        /**
         * @brief Distinct regions of the mesh intersect each other.
         */
        SelfIntersection,

        /**
         * @brief A geometric feature is smaller than the minimum supported by
         * the active print profile.
         */
        MinimumFeatureSize,

        /**
         * @brief Local wall thickness is below the required printable
         * thickness.
         */
        ThinWall,

        /**
         * @brief Surface orientation exceeds the supported overhang limit.
         */
        Overhang,

        /**
         * @brief Geometry is expected to require support for the active
         * manufacturing process and orientation.
         */
        SupportRequired
    };

}