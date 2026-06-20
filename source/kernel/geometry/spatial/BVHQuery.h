#pragma once

#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/geometry/spatial/BVH.h"
#include "kernel/math/Intersections.h"
#include "kernel/math/Ray.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace locus::kernel::geometry {

	class BVHQuery {
	public:
		[[nodiscard]] static SelectionHit raycast_faces(
			const BVH& bvh,
			const math::Ray& ray,
			float maxDistance = std::numeric_limits<float>::max()
		) {
			if (!bvh.is_valid()) {
				return SelectionHit::miss();
			}

			SelectionHit best = SelectionHit::miss();
			float bestDistance = maxDistance;

			raycast_node(bvh, bvh.root(), ray, bestDistance, best);

			return best;
		}

		[[nodiscard]] static std::vector<FaceHandle> query_bounds(
			const BVH& bvh,
			const math::Bounds& bounds
		) {
			std::vector<FaceHandle> result;

			if (!bvh.is_valid() || !bounds.is_valid()) {
				return result;
			}

			query_bounds_node(bvh, bvh.root(), bounds, result);

			return result;
		}

		[[nodiscard]] static bool intersects_bounds(
			const BVH& bvh,
			const math::Bounds& bounds
		) {
			if (!bvh.is_valid() || !bounds.is_valid()) {
				return false;
			}

			return intersects_bounds_node(bvh, bvh.root(), bounds);
		}

	private:
		static void raycast_node(
			const BVH& bvh,
			std::uint32_t nodeIndex,
			const math::Ray& ray,
			float& bestDistance,
			SelectionHit& best
		) {
			const BVHNode& node = bvh.node(nodeIndex);
			const math::RayHit boundsHit = math::intersect_ray_bounds(ray, node.bounds);

			if (!boundsHit.hit || boundsHit.distance > bestDistance) {
				return;
			}

			if (node.is_leaf()) {
				raycast_leaf(bvh, node, ray, bestDistance, best);
				return;
			}

			const BVHNode& left = bvh.node(node.left);
			const BVHNode& right = bvh.node(node.right);

			const math::RayHit leftHit = math::intersect_ray_bounds(ray, left.bounds);
			const math::RayHit rightHit = math::intersect_ray_bounds(ray, right.bounds);

			if (leftHit.hit && rightHit.hit) {
				if (leftHit.distance <= rightHit.distance) {
					raycast_node(bvh, node.left, ray, bestDistance, best);
					raycast_node(bvh, node.right, ray, bestDistance, best);
				}
				else {
					raycast_node(bvh, node.right, ray, bestDistance, best);
					raycast_node(bvh, node.left, ray, bestDistance, best);
				}

				return;
			}

			if (leftHit.hit) {
				raycast_node(bvh, node.left, ray, bestDistance, best);
			}

			if (rightHit.hit) {
				raycast_node(bvh, node.right, ray, bestDistance, best);
			}
		}

		static void raycast_leaf(
			const BVH& bvh,
			const BVHNode& node,
			const math::Ray& ray,
			float& bestDistance,
			SelectionHit& best
		) {
			for (std::uint32_t i = 0; i < node.triangleCount; ++i) {
				const BVHTriangle& triangle = bvh.triangle(node.firstTriangle + i);
				const math::RayHit hit = math::intersect_ray_triangle(ray, triangle.a, triangle.b, triangle.c);

				if (!hit.hit || hit.distance >= bestDistance) {
					continue;
				}

				bestDistance = hit.distance;
				best = SelectionHit::face_hit(
					triangle.face,
					hit.distance,
					hit.position,
					triangle.normal
				);
			}
		}

		static void query_bounds_node(
			const BVH& bvh,
			std::uint32_t nodeIndex,
			const math::Bounds& bounds,
			std::vector<FaceHandle>& result
		) {
			const BVHNode& node = bvh.node(nodeIndex);

			if (!math::intersects(node.bounds, bounds)) {
				return;
			}

			if (node.is_leaf()) {
				for (std::uint32_t i = 0; i < node.triangleCount; ++i) {
					const BVHTriangle& triangle = bvh.triangle(node.firstTriangle + i);

					if (!math::intersects(triangle.bounds, bounds) || contains(result, triangle.face)) {
						continue;
					}

					result.push_back(triangle.face);
				}

				return;
			}

			query_bounds_node(bvh, node.left, bounds, result);
			query_bounds_node(bvh, node.right, bounds, result);
		}

		[[nodiscard]] static bool intersects_bounds_node(
			const BVH& bvh,
			std::uint32_t nodeIndex,
			const math::Bounds& bounds
		) {
			const BVHNode& node = bvh.node(nodeIndex);

			if (!math::intersects(node.bounds, bounds)) {
				return false;
			}

			if (node.is_leaf()) {
				for (std::uint32_t i = 0; i < node.triangleCount; ++i) {
					const BVHTriangle& triangle = bvh.triangle(node.firstTriangle + i);

					if (math::intersects(triangle.bounds, bounds)) {
						return true;
					}
				}

				return false;
			}

			return intersects_bounds_node(bvh, node.left, bounds)
				|| intersects_bounds_node(bvh, node.right, bounds);
		}

		[[nodiscard]] static bool contains(const std::vector<FaceHandle>& handles, FaceHandle handle) {
			return std::find(handles.begin(), handles.end(), handle) != handles.end();
		}
	};

}