/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/LEMTypes.h"

#include <glm/glm.hpp>

#include <limits>

namespace locus::kernel::geometry {

    /**
     * @brief Result of a mesh selection or picking query.
     */
    struct SelectionHit {
        /**
         * @brief True when the query hit a selectable element.
         */
        bool hit = false;
        /**
         * @brief Element category represented by this hit.
         */
        LEMElementType type = LEMElementType::Vertex;

        /**
         * @brief Vertex handle when type is Vertex.
         */
        VertexHandle vertex{};
        /**
         * @brief Edge handle when type is Edge.
         */
        EdgeHandle edge{};
        /**
         * @brief Loop handle when type is Loop.
         */
        LoopHandle loop{};
        /**
         * @brief Face handle when type is Face.
         */
        FaceHandle face{};

        /**
         * @brief Query distance to the hit point.
         */
        float distance = std::numeric_limits<float>::max();
        /**
         * @brief Hit position in the query coordinate space.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        /**
         * @brief Surface or fallback normal at the hit point.
         */
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };

        /**
         * @brief Checks whether this hit references a vertex.
         *
         * @return True when the hit is a valid vertex hit.
         */
        [[nodiscard]] bool is_vertex() const
        {
            return hit && type == LEMElementType::Vertex && vertex.is_valid();
        }

        /**
         * @brief Checks whether this hit references an edge.
         *
         * @return True when the hit is a valid edge hit.
         */
        [[nodiscard]] bool is_edge() const
        {
            return hit && type == LEMElementType::Edge && edge.is_valid();
        }

        /**
         * @brief Checks whether this hit references a loop.
         *
         * @return True when the hit is a valid loop hit.
         */
        [[nodiscard]] bool is_loop() const
        {
            return hit && type == LEMElementType::Loop && loop.is_valid();
        }

        /**
         * @brief Checks whether this hit references a face.
         *
         * @return True when the hit is a valid face hit.
         */
        [[nodiscard]] bool is_face() const
        {
            return hit && type == LEMElementType::Face && face.is_valid();
        }

        /**
         * @brief Creates an empty miss result.
         *
         * @return Selection result with hit set to false.
         */
        [[nodiscard]] static SelectionHit miss()
        {
            return {};
        }

        /**
         * @brief Creates a vertex selection hit.
         *
         * @param vertex Hit vertex.
         * @param distance Query distance to the hit.
         * @param position Hit position.
         * @param normal Hit normal or fallback normal.
         * @return Selection result for the vertex.
         */
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

        /**
         * @brief Creates an edge selection hit.
         *
         * @param edge Hit edge.
         * @param distance Query distance to the hit.
         * @param position Hit position.
         * @param normal Hit normal or fallback normal.
         * @return Selection result for the edge.
         */
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

        /**
         * @brief Creates a loop selection hit.
         *
         * @param loop Hit loop.
         * @param distance Query distance to the hit.
         * @param position Hit position.
         * @param normal Hit normal or fallback normal.
         * @return Selection result for the loop.
         */
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

        /**
         * @brief Creates a face selection hit.
         *
         * @param face Hit face.
         * @param distance Query distance to the hit.
         * @param position Hit position.
         * @param normal Hit normal or fallback normal.
         * @return Selection result for the face.
         */
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
