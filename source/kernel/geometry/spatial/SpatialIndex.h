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

	class SpatialIndex {
	public:
		SpatialIndex() = default;

		explicit SpatialIndex(const LEM& mesh) {
			rebuild(mesh);
		}

		void clear() {
			bvh_.clear();
		}

		void rebuild(const LEM& mesh, const BVHBuildSettings& settings = {}) {
			BVHBuilder::build_into(mesh, bvh_, settings);
		}

		[[nodiscard]] bool empty() const {
			return bvh_.empty();
		}

		[[nodiscard]] bool is_valid() const {
			return bvh_.is_valid();
		}

		[[nodiscard]] const BVH& bvh() const {
			return bvh_;
		}

		[[nodiscard]] SelectionHit raycast_faces(
			const math::Ray& ray,
			float maxDistance = std::numeric_limits<float>::max()
		) const {
			return BVHQuery::raycast_faces(bvh_, ray, maxDistance);
		}

		[[nodiscard]] std::vector<FaceHandle> query_bounds(const math::Bounds& bounds) const {
			return BVHQuery::query_bounds(bvh_, bounds);
		}

		[[nodiscard]] bool intersects_bounds(const math::Bounds& bounds) const {
			return BVHQuery::intersects_bounds(bvh_, bounds);
		}

	private:
		BVH bvh_;
	};

}