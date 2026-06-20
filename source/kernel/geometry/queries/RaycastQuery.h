/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/queries/BoundsQuery.h"
#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/math/Intersections.h"
#include "kernel/math/Ray.h"

#include <glm/glm.hpp>

#include <limits>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Performs ray-based picking and intersection queries against LEM geometry.
     */
    class RaycastQuery {
    public:
        /**
         * @brief Raycasts visible faces and returns the nearest face hit.
         *
         * Polygon faces are tested as a triangle fan using the first face vertex
         * as the fan anchor.
         *
         * @param mesh Mesh to query.
         * @param ray Object-space ray.
         * @param maxDistance Maximum accepted hit distance.
         * @return Nearest face hit, or a miss when no face is intersected.
         */
        [[nodiscard]] static SelectionHit raycast_faces(
            const LEM& mesh,
            const locus::kernel::math::Ray& ray,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistance = maxDistance;

            for (FaceHandle faceHandle : TopologyTraversal::faces(mesh)) {
                const Face& face = mesh.face(faceHandle);
                if (face.hidden) {
                    continue;
                }

                const std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, faceHandle);
                if (vertices.size() < 3) {
                    continue;
                }

                for (std::size_t i = 1; i + 1 < vertices.size(); ++i) {
                    if (!mesh.is_valid(vertices[0]) || !mesh.is_valid(vertices[i]) || !mesh.is_valid(vertices[i + 1])) {
                        continue;
                    }

                    const glm::vec3 a = mesh.vertex(vertices[0]).position;
                    const glm::vec3 b = mesh.vertex(vertices[i]).position;
                    const glm::vec3 c = mesh.vertex(vertices[i + 1]).position;

                    const locus::kernel::math::RayHit hit =
                        locus::kernel::math::intersect_ray_triangle(ray, a, b, c);

                    if (!hit.hit || hit.distance >= bestDistance) {
                        continue;
                    }

                    bestDistance = hit.distance;
                    best = SelectionHit::face_hit(
                        faceHandle,
                        hit.distance,
                        hit.position,
                        face.normal
                    );
                }
            }

            return best;
        }

        /**
         * @brief Raycasts visible vertices using a cylindrical pick radius.
         *
         * @param mesh Mesh to query.
         * @param ray Object-space ray.
         * @param radius Maximum perpendicular distance from the ray.
         * @param maxDistance Maximum accepted distance along the ray.
         * @return Nearest vertex hit, or a miss when no vertex is close enough.
         */
        [[nodiscard]] static SelectionHit raycast_vertices(
            const LEM& mesh,
            const locus::kernel::math::Ray& ray,
            float radius,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistance = maxDistance;

            if (radius <= 0.0f) {
                return best;
            }

            const locus::kernel::math::Ray normalizedRay = ray.normalized();

            for (VertexHandle vertexHandle : TopologyTraversal::vertices(mesh)) {
                const Vertex& vertex = mesh.vertex(vertexHandle);
                if (vertex.hidden) {
                    continue;
                }

                float distanceAlongRay = 0.0f;
                const float distanceToRay = distance_point_to_ray(
                    vertex.position,
                    normalizedRay,
                    distanceAlongRay
                );

                if (distanceAlongRay < 0.0f || distanceAlongRay >= bestDistance || distanceToRay > radius) {
                    continue;
                }

                bestDistance = distanceAlongRay;
                best = SelectionHit::vertex_hit(
                    vertexHandle,
                    distanceAlongRay,
                    vertex.position
                );
            }

            return best;
        }

        /**
         * @brief Raycasts visible edges using a cylindrical pick radius.
         *
         * @param mesh Mesh to query.
         * @param ray Object-space ray.
         * @param radius Maximum distance between ray and edge segment.
         * @param maxDistance Maximum accepted distance along the ray.
         * @return Nearest edge hit, or a miss when no edge is close enough.
         */
        [[nodiscard]] static SelectionHit raycast_edges(
            const LEM& mesh,
            const locus::kernel::math::Ray& ray,
            float radius,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistance = maxDistance;

            if (radius <= 0.0f) {
                return best;
            }

            const locus::kernel::math::Ray normalizedRay = ray.normalized();

            for (EdgeHandle edgeHandle : TopologyTraversal::edges(mesh)) {
                const Edge& edge = mesh.edge(edgeHandle);
                if (edge.hidden || !mesh.is_valid(edge.vertexA) || !mesh.is_valid(edge.vertexB)) {
                    continue;
                }

                const glm::vec3 a = mesh.vertex(edge.vertexA).position;
                const glm::vec3 b = mesh.vertex(edge.vertexB).position;

                SegmentRayDistance distance = distance_segment_to_ray(a, b, normalizedRay);

                if (distance.rayDistance < 0.0f || distance.rayDistance >= bestDistance || distance.distance > radius) {
                    continue;
                }

                bestDistance = distance.rayDistance;
                best = SelectionHit::edge_hit(
                    edgeHandle,
                    distance.rayDistance,
                    distance.segmentPoint
                );
            }

            return best;
        }

        /**
         * @brief Raycasts faces, edges, and vertices and returns the nearest selectable element.
         *
         * @param mesh Mesh to query.
         * @param ray Object-space ray.
         * @param vertexRadius Vertex pick radius.
         * @param edgeRadius Edge pick radius.
         * @param maxDistance Maximum accepted distance along the ray.
         * @return Nearest element hit, or a miss when nothing is hit.
         */
        [[nodiscard]] static SelectionHit raycast_element(
            const LEM& mesh,
            const locus::kernel::math::Ray& ray,
            float vertexRadius,
            float edgeRadius,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = raycast_faces(mesh, ray, maxDistance);

            if (best.hit) {
                maxDistance = best.distance;
            }

            const SelectionHit edgeHit = raycast_edges(mesh, ray, edgeRadius, maxDistance);
            if (edgeHit.hit) {
                best = edgeHit;
                maxDistance = edgeHit.distance;
            }

            const SelectionHit vertexHit = raycast_vertices(mesh, ray, vertexRadius, maxDistance);
            if (vertexHit.hit) {
                best = vertexHit;
            }

            return best;
        }

        /**
         * @brief Intersects a ray with the visible mesh bounds.
         *
         * @param mesh Mesh whose visible bounds are tested.
         * @param ray Object-space ray.
         * @return Ray hit data for the mesh bounds.
         */
        [[nodiscard]] static locus::kernel::math::RayHit raycast_mesh_bounds(
            const LEM& mesh,
            const locus::kernel::math::Ray& ray
        )
        {
            return locus::kernel::math::intersect_ray_bounds(
                ray,
                BoundsQuery::mesh_bounds(mesh)
            );
        }

    private:
        /**
         * @brief Closest-distance data between a segment and a ray.
         */
        struct SegmentRayDistance {
            /**
             * @brief Shortest distance between the segment and the ray.
             */
            float distance = std::numeric_limits<float>::max();
            /**
             * @brief Distance from ray origin to the closest ray point.
             */
            float rayDistance = std::numeric_limits<float>::max();
            /**
             * @brief Normalized distance from segment start to the closest segment point.
             */
            float segmentDistance = 0.0f;
            /**
             * @brief Closest point on the ray.
             */
            glm::vec3 rayPoint{ 0.0f, 0.0f, 0.0f };
            /**
             * @brief Closest point on the segment.
             */
            glm::vec3 segmentPoint{ 0.0f, 0.0f, 0.0f };
        };

        /**
         * @brief Computes perpendicular distance from a point to a ray.
         *
         * @param point Object-space point.
         * @param ray Normalized object-space ray.
         * @param distanceAlongRay Receives signed distance from the ray origin.
         * @return Perpendicular distance from point to ray.
         */
        [[nodiscard]] static float distance_point_to_ray(
            const glm::vec3& point,
            const locus::kernel::math::Ray& ray,
            float& distanceAlongRay
        )
        {
            const glm::vec3 toPoint = point - ray.origin;
            distanceAlongRay = glm::dot(toPoint, ray.direction);

            const glm::vec3 closestPoint = ray.origin + ray.direction * distanceAlongRay;
            return glm::length(point - closestPoint);
        }

        /**
         * @brief Computes closest-distance data between a segment and a ray.
         *
         * @param a First segment endpoint.
         * @param b Second segment endpoint.
         * @param ray Normalized object-space ray.
         * @return Closest-distance data for the segment and ray.
         */
        [[nodiscard]] static SegmentRayDistance distance_segment_to_ray(
            const glm::vec3& a,
            const glm::vec3& b,
            const locus::kernel::math::Ray& ray
        )
        {
            const glm::vec3 segmentDirection = b - a;
            const glm::vec3 rayDirection = ray.direction;
            const glm::vec3 delta = a - ray.origin;

            const float segmentLengthSquared = glm::dot(segmentDirection, segmentDirection);
            const float rayLengthSquared = glm::dot(rayDirection, rayDirection);
            const float segmentRayDot = glm::dot(segmentDirection, rayDirection);
            const float segmentDeltaDot = glm::dot(segmentDirection, delta);
            const float rayDeltaDot = glm::dot(rayDirection, delta);

            float segmentT = 0.0f;
            float rayT = 0.0f;

            const float denominator = segmentLengthSquared * rayLengthSquared - segmentRayDot * segmentRayDot;

            if (segmentLengthSquared <= 0.0f) {
                rayT = -rayDeltaDot / rayLengthSquared;
                segmentT = 0.0f;
            }
            else if (denominator > 0.0f) {
                segmentT = (segmentRayDot * rayDeltaDot - rayLengthSquared * segmentDeltaDot) / denominator;
                segmentT = glm::clamp(segmentT, 0.0f, 1.0f);
                rayT = (segmentRayDot * segmentT + rayDeltaDot) / rayLengthSquared;
            }
            else {
                segmentT = 0.0f;
                rayT = rayDeltaDot / rayLengthSquared;
            }

            if (rayT < 0.0f) {
                rayT = 0.0f;

                if (segmentLengthSquared > 0.0f) {
                    segmentT = glm::clamp(-segmentDeltaDot / segmentLengthSquared, 0.0f, 1.0f);
                }
                else {
                    segmentT = 0.0f;
                }
            }

            SegmentRayDistance result;
            result.rayDistance = rayT;
            result.segmentDistance = segmentT;
            result.rayPoint = ray.origin + rayDirection * rayT;
            result.segmentPoint = a + segmentDirection * segmentT;
            result.distance = glm::length(result.rayPoint - result.segmentPoint);
            return result;
        }
    };

}
