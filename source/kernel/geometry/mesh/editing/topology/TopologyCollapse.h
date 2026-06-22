/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstddef>
#include <vector>

#include <glm/vec3.hpp>

namespace locus::kernel::geometry {

	class LEM;

	namespace editing::topology {

		/**
		 * @brief Collapses an edge by merging its endpoints into one vertex.
		 *
		 * @param mesh Mesh that owns the edge.
		 * @param diff Diff recorder that receives topology events.
		 * @param edgeHandle Edge to collapse.
		 * @return True when the collapse succeeded.
		 */
		bool collapse_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle);

        /**
         * @brief Merges one vertex into another, even when they are not connected by an edge.
         *
         * All faces using sourceVertex are rebuilt to reference targetVertex.
         * Degenerate faces created by the merge are removed instead of rebuilt.
         *
         * @param mesh Mesh that owns the vertices.
         * @param diff Diff recorder that receives topology events.
         * @param sourceVertex Vertex that will be removed.
         * @param targetVertex Vertex that will receive the merged topology.
         * @return True when the merge succeeded.
         */
        bool merge_vertices(
            LEM& mesh,
            LEMDiff& diff,
            VertexHandle sourceVertex,
            VertexHandle targetVertex);

        /**
         * @brief Merges one vertex into another and assigns the final target position.
         *
         * @param mesh Mesh that owns the vertices.
         * @param diff Diff recorder that receives topology events.
         * @param sourceVertex Vertex that will be removed.
         * @param targetVertex Vertex that will receive the merged topology.
         * @param position Final object-space position assigned to targetVertex.
         * @return True when the merge succeeded.
         */
        bool merge_vertices_at_position(
            LEM& mesh,
            LEMDiff& diff,
            VertexHandle sourceVertex,
            VertexHandle targetVertex,
            const glm::vec3& position);

        /**
         * @brief Merges all active vertices that are closer than a distance threshold.
         *
         * @param mesh Mesh that owns the vertices.
         * @param diff Diff recorder that receives topology events.
         * @param distance Maximum distance between vertices to merge.
         * @return Number of successful vertex merges.
         */
        std::size_t merge_vertices_by_distance(
            LEM& mesh,
            LEMDiff& diff,
            float distance);

        /**
         * @brief Merges vertices from a restricted vertex set using a distance threshold.
         *
         * @param mesh Mesh that owns the vertices.
         * @param diff Diff recorder that receives topology events.
         * @param vertices Candidate vertices to weld.
         * @param distance Maximum distance between vertices to merge.
         * @return Number of successful vertex merges.
         */
        std::size_t weld_vertices(
            LEM& mesh,
            LEMDiff& diff,
            const std::vector<VertexHandle>& vertices,
            float distance);

		/**
		 * @brief Dissolves an edge while trying to preserve the surrounding region.
		 *
		 * @param mesh Mesh that owns the edge.
		 * @param diff Diff recorder that receives topology events.
		 * @param edgeHandle Edge to dissolve.
		 * @return True when the edge was dissolved or safely removed.
		 */
		bool dissolve_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle);

		/**
		 * @brief Dissolves a vertex when it can be removed without invalid topology.
		 *
		 * @param mesh Mesh that owns the vertex.
		 * @param diff Diff recorder that receives topology events.
		 * @param vertexHandle Vertex to dissolve.
		 * @return True when the vertex was dissolved or safely removed.
		 */
		bool dissolve_vertex(LEM& mesh, LEMDiff& diff, VertexHandle vertexHandle);

		/**
		 * @brief Dissolves a face and removes loose boundary elements left behind.
		 *
		 * @param mesh Mesh that owns the face.
		 * @param diff Diff recorder that receives topology events.
		 * @param faceHandle Face to dissolve.
		 * @return True when the face existed and was dissolved.
		 */
		bool dissolve_face(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle);

	}

}