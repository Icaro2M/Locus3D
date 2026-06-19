#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/queries/ProximityQuery.h"
#include "kernel/geometry/queries/RaycastQuery.h"
#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/math/Ray.h"

#include <glm/glm.hpp>

#include <limits>

namespace locus::kernel::geometry {

    enum class SelectionElementMask : unsigned int {
        None = 0,
        Vertex = 1u << 0,
        Edge = 1u << 1,
        Face = 1u << 2,
        All = Vertex | Edge | Face
    };

    [[nodiscard]] constexpr SelectionElementMask operator|(SelectionElementMask lhs, SelectionElementMask rhs)
    {
        return static_cast<SelectionElementMask>(
            static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs)
            );
    }

    [[nodiscard]] constexpr SelectionElementMask operator&(SelectionElementMask lhs, SelectionElementMask rhs)
    {
        return static_cast<SelectionElementMask>(
            static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs)
            );
    }

    [[nodiscard]] constexpr bool has_selection_mask(SelectionElementMask mask, SelectionElementMask value)
    {
        return (static_cast<unsigned int>(mask) & static_cast<unsigned int>(value)) != 0u;
    }

    struct SelectionQueryOptions {
        SelectionElementMask mask = SelectionElementMask::All;
        float maxDistance = std::numeric_limits<float>::max();
        float vertexRadius = 0.05f;
        float edgeRadius = 0.025f;
        bool preferVertices = true;
        bool preferEdges = true;
    };

    class SelectionQuery {
    public:
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