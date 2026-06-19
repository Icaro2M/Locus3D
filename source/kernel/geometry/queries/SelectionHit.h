#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/LEMTypes.h"

#include <glm/glm.hpp>

#include <limits>

namespace locus::kernel::geometry {

    struct SelectionHit {
        bool hit = false;
        LEMElementType type = LEMElementType::Vertex;

        VertexHandle vertex{};
        EdgeHandle edge{};
        LoopHandle loop{};
        FaceHandle face{};

        float distance = std::numeric_limits<float>::max();
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };

        [[nodiscard]] bool is_vertex() const
        {
            return hit && type == LEMElementType::Vertex && vertex.is_valid();
        }

        [[nodiscard]] bool is_edge() const
        {
            return hit && type == LEMElementType::Edge && edge.is_valid();
        }

        [[nodiscard]] bool is_loop() const
        {
            return hit && type == LEMElementType::Loop && loop.is_valid();
        }

        [[nodiscard]] bool is_face() const
        {
            return hit && type == LEMElementType::Face && face.is_valid();
        }

        [[nodiscard]] static SelectionHit miss()
        {
            return {};
        }

        [[nodiscard]] static SelectionHit vertex_hit(
            VertexHandle vertex,
            float distance,
            const glm::vec3& position,
            const glm::vec3& normal = glm::vec3{ 0.0f, 1.0f, 0.0f }
        )
        {
            SelectionHit result;
            result.hit = vertex.is_valid();
            result.type = LEMElementType::Vertex;
            result.vertex = vertex;
            result.distance = distance;
            result.position = position;
            result.normal = normal;
            return result;
        }

        [[nodiscard]] static SelectionHit edge_hit(
            EdgeHandle edge,
            float distance,
            const glm::vec3& position,
            const glm::vec3& normal = glm::vec3{ 0.0f, 1.0f, 0.0f }
        )
        {
            SelectionHit result;
            result.hit = edge.is_valid();
            result.type = LEMElementType::Edge;
            result.edge = edge;
            result.distance = distance;
            result.position = position;
            result.normal = normal;
            return result;
        }

        [[nodiscard]] static SelectionHit loop_hit(
            LoopHandle loop,
            float distance,
            const glm::vec3& position,
            const glm::vec3& normal = glm::vec3{ 0.0f, 1.0f, 0.0f }
        )
        {
            SelectionHit result;
            result.hit = loop.is_valid();
            result.type = LEMElementType::Loop;
            result.loop = loop;
            result.distance = distance;
            result.position = position;
            result.normal = normal;
            return result;
        }

        [[nodiscard]] static SelectionHit face_hit(
            FaceHandle face,
            float distance,
            const glm::vec3& position,
            const glm::vec3& normal = glm::vec3{ 0.0f, 1.0f, 0.0f }
        )
        {
            SelectionHit result;
            result.hit = face.is_valid();
            result.type = LEMElementType::Face;
            result.face = face;
            result.distance = distance;
            result.position = position;
            result.normal = normal;
            return result;
        }
    };

}