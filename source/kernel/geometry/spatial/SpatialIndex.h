/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/geometry/spatial/BVH.h"
#include "kernel/geometry/spatial/BVHBuilder.h"
#include "kernel/geometry/spatial/BVHQuery.h"
#include "kernel/math/Bounds.h"
#include "kernel/math/Ray.h"

#include <vector>

namespace locus::kernel::geometry {

	/**
	 * @brief High-level spatial index facade backed by a BVH.
	 */
	class SpatialIndex {
	public:
		SpatialIndex() = default;

		/**
		 * @brief Creates an index and immediately builds it from a mesh.
		 *
		 * @param mesh Mesh used to build the index.
		 */
		explicit SpatialIndex(const LEM& mesh) {
			rebuild(mesh);
		}

		/**
		 * @brief Clears all indexed spatial data.
		 */
		void clear() {
			bvh_.clear();
		}

		/**
		 * @brief Rebuilds the index from visible mesh faces.
		 *
		 * @param mesh Mesh used to build the index.
		 * @param settings BVH construction settings.
		 */
		void rebuild(const LEM& mesh, const BVHBuildSettings& settings = {}) {
			BVHBuilder::build_into(mesh, bvh_, settings);
		}

		/**
		 * @brief Checks whether the index contains no triangle primitives.
		 *
		 * @return True when the backing BVH is empty.
		 */
		[[nodiscard]] bool empty() const {
			return bvh_.empty();
		}

		/**
		 * @brief Checks whether the backing BVH can be queried.
		 *
		 * @return True when the backing BVH is valid.
		 */
		[[nodiscard]] bool is_valid() const {
			return bvh_.is_valid();
		}

		/**
		 * @brief Returns the backing BVH.
		 *
		 * @return Read-only BVH reference.
		 */
		[[nodiscard]] const BVH& bvh() const {
			return bvh_;
		}

		/**
		 * @brief Raycasts indexed face triangles.
		 *
		 * @param ray Object-space ray.
		 * @param maxDistance Maximum accepted hit distance.
		 * @return Nearest face hit, or a miss when nothing is hit.
		 */
		[[nodiscard]] SelectionHit raycast_faces(
			const math::Ray& ray,
			float maxDistance = std::numeric_limits<float>::max()
		) const {
			return BVHQuery::raycast_faces(bvh_, ray, maxDistance);
		}

		/**
		 * @brief Returns faces whose indexed triangle bounds overlap bounds.
		 *
		 * @param bounds Query bounds in object space.
		 * @return Unique face handles overlapping the bounds.
		 */
		[[nodiscard]] std::vector<FaceHandle> query_bounds(const math::Bounds& bounds) const {
			return BVHQuery::query_bounds(bvh_, bounds);
		}

		/**
		 * @brief Checks whether any indexed triangle bounds overlap bounds.
		 *
		 * @param bounds Query bounds in object space.
		 * @return True when at least one triangle overlaps.
		 */
		[[nodiscard]] bool intersects_bounds(const math::Bounds& bounds) const {
			return BVHQuery::intersects_bounds(bvh_, bounds);
		}

	private:
		BVH bvh_;
	};

}
