/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

#include <vector>

namespace locus::kernel::geometry {

	class LEM;

	namespace editing::topology {

		/**
		 * @brief Adds a loose vertex to a LEM mesh.
		 *
		 * @param mesh Mesh that receives the vertex.
		 * @param diff Diff recorder that receives insertion events.
		 * @param position Vertex position in object space.
		 * @return Handle referencing the created vertex.
		 */
		VertexHandle add_vertex(LEM& mesh, LEMDiff& diff, const glm::vec3& position);

		/**
		 * @brief Finds or creates a non-directional edge between two vertices.
		 *
		 * @param mesh Mesh that owns the vertices and edge.
		 * @param diff Diff recorder that receives insertion or modification events.
		 * @param vertexA First endpoint vertex.
		 * @param vertexB Second endpoint vertex.
		 * @return Handle referencing the existing or created edge.
		 */
		EdgeHandle find_or_create_edge(LEM& mesh, LEMDiff& diff, VertexHandle vertexA, VertexHandle vertexB);

		/**
		 * @brief Adds a polygonal face to a LEM mesh.
		 *
		 * @param mesh Mesh that receives the face.
		 * @param diff Diff recorder that receives insertion or modification events.
		 * @param vertices Ordered face vertices.
		 * @return Handle referencing the created face, or an invalid handle on failure.
		 */
		FaceHandle add_face(LEM& mesh, LEMDiff& diff, const std::vector<VertexHandle>& vertices);

	}

}