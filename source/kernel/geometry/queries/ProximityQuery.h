#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/math/GeometryMath.h"

#include <glm/glm.hpp>

#include <cmath>
#include <limits>
#include <vector>

namespace locus::kernel::geometry {

    class ProximityQuery {
    public:
        [[nodiscard]] static SelectionHit closest_vertex(
            const LEM& mesh,
            const glm::vec3& point,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistanceSquared = maxDistance * maxDistance;

            for (VertexHandle vertexHandle : TopologyTraversal::vertices(mesh)) {
                const Vertex& vertex = mesh.vertex(vertexHandle);
                if (vertex.hidden) {
                    continue;
                }

                const glm::vec3 offset = vertex.position - point;
                const float distanceSquared = glm::dot(offset, offset);

                if (distanceSquared >= bestDistanceSquared) {
                    continue;
                }

                bestDistanceSquared = distanceSquared;
                best = SelectionHit::vertex_hit(
                    vertexHandle,
                    std::sqrt(distanceSquared),
                    vertex.position
                );
            }

            return best;
        }

        [[nodiscard]] static SelectionHit closest_edge(
            const LEM& mesh,
            const glm::vec3& point,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistanceSquared = maxDistance * maxDistance;

            for (EdgeHandle edgeHandle : TopologyTraversal::edges(mesh)) {
                const Edge& edge = mesh.edge(edgeHandle);
                if (edge.hidden || !mesh.is_valid(edge.vertexA) || !mesh.is_valid(edge.vertexB)) {
                    continue;
                }

                const glm::vec3 a = mesh.vertex(edge.vertexA).position;
                const glm::vec3 b = mesh.vertex(edge.vertexB).position;
                const glm::vec3 closestPoint = closest_point_on_segment(point, a, b);
                const glm::vec3 offset = closestPoint - point;
                const float distanceSquared = glm::dot(offset, offset);

                if (distanceSquared >= bestDistanceSquared) {
                    continue;
                }

                bestDistanceSquared = distanceSquared;
                best = SelectionHit::edge_hit(
                    edgeHandle,
                    std::sqrt(distanceSquared),
                    closestPoint
                );
            }

            return best;
        }

        [[nodiscard]] static SelectionHit closest_face(
            const LEM& mesh,
            const glm::vec3& point,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = SelectionHit::miss();
            float bestDistanceSquared = maxDistance * maxDistance;

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

                    const glm::vec3 closestPoint = closest_point_on_triangle(point, a, b, c);
                    const glm::vec3 offset = closestPoint - point;
                    const float distanceSquared = glm::dot(offset, offset);

                    if (distanceSquared >= bestDistanceSquared) {
                        continue;
                    }

                    bestDistanceSquared = distanceSquared;
                    best = SelectionHit::face_hit(
                        faceHandle,
                        std::sqrt(distanceSquared),
                        closestPoint,
                        face.normal
                    );
                }
            }

            return best;
        }

        [[nodiscard]] static SelectionHit closest_element(
            const LEM& mesh,
            const glm::vec3& point,
            float maxDistance = std::numeric_limits<float>::max()
        )
        {
            SelectionHit best = SelectionHit::miss();

            const SelectionHit vertexHit = closest_vertex(mesh, point, maxDistance);
            if (vertexHit.hit) {
                best = vertexHit;
                maxDistance = vertexHit.distance;
            }

            const SelectionHit edgeHit = closest_edge(mesh, point, maxDistance);
            if (edgeHit.hit) {
                best = edgeHit;
                maxDistance = edgeHit.distance;
            }

            const SelectionHit faceHit = closest_face(mesh, point, maxDistance);
            if (faceHit.hit) {
                best = faceHit;
            }

            return best;
        }

    private:
        [[nodiscard]] static glm::vec3 closest_point_on_segment(
            const glm::vec3& point,
            const glm::vec3& a,
            const glm::vec3& b
        )
        {
            const glm::vec3 ab = b - a;
            const float lengthSquared = glm::dot(ab, ab);

            if (lengthSquared <= 0.0f) {
                return a;
            }

            const float t = locus::kernel::math::clamp01(glm::dot(point - a, ab) / lengthSquared);
            return a + ab * t;
        }

        [[nodiscard]] static glm::vec3 closest_point_on_triangle(
            const glm::vec3& point,
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c
        )
        {
            const glm::vec3 ab = b - a;
            const glm::vec3 ac = c - a;
            const glm::vec3 ap = point - a;

            const float d1 = glm::dot(ab, ap);
            const float d2 = glm::dot(ac, ap);

            if (d1 <= 0.0f && d2 <= 0.0f) {
                return a;
            }

            const glm::vec3 bp = point - b;
            const float d3 = glm::dot(ab, bp);
            const float d4 = glm::dot(ac, bp);

            if (d3 >= 0.0f && d4 <= d3) {
                return b;
            }

            const float vc = d1 * d4 - d3 * d2;
            if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
                const float v = d1 / (d1 - d3);
                return a + ab * v;
            }

            const glm::vec3 cp = point - c;
            const float d5 = glm::dot(ab, cp);
            const float d6 = glm::dot(ac, cp);

            if (d6 >= 0.0f && d5 <= d6) {
                return c;
            }

            const float vb = d5 * d2 - d1 * d6;
            if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
                const float w = d2 / (d2 - d6);
                return a + ac * w;
            }

            const float va = d3 * d6 - d5 * d4;
            if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
                const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
                return b + (c - b) * w;
            }

            const float denominator = 1.0f / (va + vb + vc);
            const float v = vb * denominator;
            const float w = vc * denominator;

            return a + ab * v + ac * w;
        }
    };

}