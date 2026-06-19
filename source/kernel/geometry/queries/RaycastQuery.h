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

    class RaycastQuery {
    public:
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
        struct SegmentRayDistance {
            float distance = std::numeric_limits<float>::max();
            float rayDistance = std::numeric_limits<float>::max();
            float segmentDistance = 0.0f;
            glm::vec3 rayPoint{ 0.0f, 0.0f, 0.0f };
            glm::vec3 segmentPoint{ 0.0f, 0.0f, 0.0f };
        };

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