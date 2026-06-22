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
		 * @brief Removes a face and its boundary loops while keeping edges and vertices alive.
		 *
		 * @param mesh Mesh that owns the face.
		 * @param diff Diff recorder that receives removal events.
		 * @param faceHandle Face to remove.
		 * @return True when the face existed and was removed.
		 */
		bool remove_face(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle);

		/**
		 * @brief Removes an edge only when no active loop references it.
		 *
		 * @param mesh Mesh that owns the edge.
		 * @param diff Diff recorder that receives removal events.
		 * @param edgeHandle Edge to remove.
		 * @return True when the edge existed and was loose.
		 */
		bool remove_edge_if_loose(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle);

		/**
		 * @brief Removes a vertex only when no active edge or loop references it.
		 *
		 * @param mesh Mesh that owns the vertex.
		 * @param diff Diff recorder that receives removal events.
		 * @param vertexHandle Vertex to remove.
		 * @return True when the vertex existed and was loose.
		 */
		bool remove_vertex_if_loose(LEM& mesh, LEMDiff& diff, VertexHandle vertexHandle);

		/**
		 * @brief Marks a face slot as deleted without deleting its boundary loops.
		 *
		 * @param mesh Mesh that owns the face.
		 * @param diff Diff recorder that receives removal events.
		 * @param faceHandle Face to mark as deleted.
		 * @return True when the face existed and was killed.
		 */
		bool kill_face_only(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle);

		/**
		 * @brief Marks an edge slot as deleted without deleting its vertices.
		 *
		 * @param mesh Mesh that owns the edge.
		 * @param diff Diff recorder that receives removal events.
		 * @param edgeHandle Edge to mark as deleted.
		 * @return True when the edge existed and was killed.
		 */
		bool kill_edge_only(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle);

		/**
		 * @brief Removes a loop from face and radial cycles and marks it as deleted.
		 *
		 * @param mesh Mesh that owns the loop.
		 * @param diff Diff recorder that receives removal events.
		 * @param loopHandle Loop to kill.
		 * @return True when the loop existed and was killed.
		 */
		bool kill_loop(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle);

	}

}