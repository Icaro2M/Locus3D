/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/queries/ProximityQuery.h"
#include "kernel/geometry/queries/RaycastQuery.h"
#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/math/Ray.h"

#include <glm/glm.hpp>

#include <limits>

namespace locus::kernel::geometry {

    /**
     * @brief Bitmask of mesh element types allowed by a selection query.
     */
    enum class SelectionElementMask : unsigned int {
        /**
         * @brief No element types are selectable.
         */
        None = 0,
        /**
         * @brief Vertices are selectable.
         */
        Vertex = 1u << 0,
        /**
         * @brief Edges are selectable.
         */
        Edge = 1u << 1,
        /**
         * @brief Faces are selectable.
         */
        Face = 1u << 2,
        /**
         * @brief Vertices, edges, and faces are selectable.
         */
        All = Vertex | Edge | Face
    };

    /**
     * @brief Combines two selection element masks.
     *
     * @param lhs First mask.
     * @param rhs Second mask.
     * @return Combined mask.
     */
    [[nodiscard]] constexpr SelectionElementMask operator|(SelectionElementMask lhs, SelectionElementMask rhs)
    {
        return static_cast<SelectionElementMask>(
            static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs)
            );
    }

    /**
     * @brief Intersects two selection element masks.
     *
     * @param lhs First mask.
     * @param rhs Second mask.
     * @return Mask containing only shared bits.
     */
    [[nodiscard]] constexpr SelectionElementMask operator&(SelectionElementMask lhs, SelectionElementMask rhs)
    {
        return static_cast<SelectionElementMask>(
            static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs)
            );
    }

    /**
     * @brief Checks whether a selection mask contains a requested element type.
     *
     * @param mask Mask to test.
     * @param value Element mask to find.
     * @return True when value is present in mask.
     */
    [[nodiscard]] constexpr bool has_selection_mask(SelectionElementMask mask, SelectionElementMask value)
    {
        return (static_cast<unsigned int>(mask) & static_cast<unsigned int>(value)) != 0u;
    }

    /**
     * @brief Options used by point and ray selection queries.
     */
    struct SelectionQueryOptions {
        /**
         * @brief Element types that may be selected.
         */
        SelectionElementMask mask = SelectionElementMask::All;
        /**
         * @brief Maximum accepted hit or proximity distance.
         */
        float maxDistance = std::numeric_limits<float>::max();
        /**
         * @brief Vertex pick radius for ray selection.
         */
        float vertexRadius = 0.05f;
        /**
         * @brief Edge pick radius for ray selection.
         */
        float edgeRadius = 0.025f;
        /**
         * @brief True when a vertex may replace an equally near current hit.
         */
        bool preferVertices = true;
        /**
         * @brief True when an edge may replace an equally near current hit.
         */
        bool preferEdges = true;
    };

    /**
     * @brief High-level selection helpers for ray and point picking.
     */
    class SelectionQuery {
    public:
        /**
         * @brief Picks a mesh element using a ray and selection options.
         *
         * @param mesh Mesh to query.
         * @param ray Object-space picking ray.
         * @param options Selection filtering and priority options.
         * @return Selected element hit, or a miss when nothing matches.
         */
        [[nodiscard]] static SelectionHit pick_by_ray(
            const LEM& mesh,
            const locus::kernel::math::Ray& ray,
            const SelectionQueryOptions& options = {}
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistance = options.maxDistance;

            if (has_selection_mask(options.mask, SelectionElementMask::Face)) {
                const SelectionHit faceHit = RaycastQuery::raycast_faces(mesh, ray, bestDistance);
                if (faceHit.hit) {
                    best = faceHit;
                    bestDistance = faceHit.distance;
                }
            }

            if (has_selection_mask(options.mask, SelectionElementMask::Edge)) {
                const SelectionHit edgeHit = RaycastQuery::raycast_edges(mesh, ray, options.edgeRadius, bestDistance);

                if (should_replace(best, edgeHit, options.preferEdges)) {
                    best = edgeHit;
                    bestDistance = edgeHit.distance;
                }
            }

            if (has_selection_mask(options.mask, SelectionElementMask::Vertex)) {
                const SelectionHit vertexHit = RaycastQuery::raycast_vertices(mesh, ray, options.vertexRadius, bestDistance);

                if (should_replace(best, vertexHit, options.preferVertices)) {
                    best = vertexHit;
                }
            }

            return best;
        }

        /**
         * @brief Picks a mesh element by proximity to a point.
         *
         * @param mesh Mesh to query.
         * @param point Object-space picking point.
         * @param options Selection filtering and priority options.
         * @return Selected element hit, or a miss when nothing matches.
         */
        [[nodiscard]] static SelectionHit pick_by_point(
            const LEM& mesh,
            const glm::vec3& point,
            const SelectionQueryOptions& options = {}
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistance = options.maxDistance;

            if (has_selection_mask(options.mask, SelectionElementMask::Face)) {
                const SelectionHit faceHit = ProximityQuery::closest_face(mesh, point, bestDistance);
                if (faceHit.hit) {
                    best = faceHit;
                    bestDistance = faceHit.distance;
                }
            }

            if (has_selection_mask(options.mask, SelectionElementMask::Edge)) {
                const SelectionHit edgeHit = ProximityQuery::closest_edge(mesh, point, bestDistance);

                if (should_replace(best, edgeHit, options.preferEdges)) {
                    best = edgeHit;
                    bestDistance = edgeHit.distance;
                }
            }

            if (has_selection_mask(options.mask, SelectionElementMask::Vertex)) {
                const SelectionHit vertexHit = ProximityQuery::closest_vertex(mesh, point, bestDistance);

                if (should_replace(best, vertexHit, options.preferVertices)) {
                    best = vertexHit;
                }
            }

            return best;
        }

    private:
        /**
         * @brief Checks whether a candidate hit should replace the current hit.
         *
         * @param current Current best hit.
         * @param candidate Candidate hit.
         * @param preferCandidate True when equal distances favor the candidate.
         * @return True when candidate should become the current hit.
         */
        [[nodiscard]] static bool should_replace(
            const SelectionHit& current,
            const SelectionHit& candidate,
            bool preferCandidate
        )
        {
            if (!candidate.hit) {
                return false;
            }

            if (!current.hit) {
                return true;
            }

            if (preferCandidate && candidate.distance <= current.distance) {
                return true;
            }

            return candidate.distance < current.distance;
        }
    };

}
