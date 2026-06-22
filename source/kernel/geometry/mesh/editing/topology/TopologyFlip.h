/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstddef>

namespace locus::kernel::geometry {

	class LEM;

	namespace editing::topology {

		/**
		 * @brief Reverses the winding of a face while preserving radial links.
		 *
		 * @param mesh Mesh that owns the face.
		 * @param diff Diff recorder that receives modification events.
		 * @param faceHandle Face whose boundary orientation will be reversed.
		 * @return True when the face existed and could be flipped.
		 */
		bool flip_face(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle);

		/**
		 * @brief Reverses the winding of every active face.
		 *
		 * @param mesh Mesh whose faces will be flipped.
		 * @param diff Diff recorder that receives modification events.
		 * @return Number of faces successfully flipped.
		 */
		std::size_t flip_all_faces(LEM& mesh, LEMDiff& diff);

		/**
		 * @brief Flips a manifold edge shared by two triangular faces.
		 *
		 * @param mesh Mesh that owns the edge.
		 * @param diff Diff recorder that receives topology events.
		 * @param edgeHandle Edge to flip.
		 * @return True when the edge was shared by two triangles and was flipped.
		 */
		bool flip_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle);

	}

}