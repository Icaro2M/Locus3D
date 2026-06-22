/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

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