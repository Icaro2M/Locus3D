/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/geometry/spatial/BVH.h"
#include "kernel/math/Intersections.h"
#include "kernel/math/Ray.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace locus::kernel::geometry {

	/**
	 * @brief Query helpers for BVH-backed face acceleration.
	 */
	class BVHQuery {
	public:
		/**
		 * @brief Raycasts BVH triangle faces and returns the nearest hit.
		 *
		 * @param bvh BVH to query.
		 * @param ray Object-space ray.
		 * @param maxDistance Maximum accepted hit distance.
		 * @return Nearest face hit, or a miss when nothing is hit.
		 */
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

		/**
		 * @brief Returns faces whose triangle bounds overlap query bounds.
		 *
		 * @param bvh BVH to query.
		 * @param bounds Query bounds in object space.
		 * @return Unique face handles overlapping the bounds.
		 */
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

		/**
		 * @brief Checks whether any triangle bounds overlap query bounds.
		 *
		 * @param bvh BVH to query.
		 * @param bounds Query bounds in object space.
		 * @return True when any indexed triangle overlaps the bounds.
		 */
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
		/**
		 * @brief Traverses a node for nearest raycast hit.
		 *
		 * @param bvh BVH to query.
		 * @param nodeIndex Current node index.
		 * @param ray Object-space ray.
		 * @param bestDistance Current nearest hit distance.
		 * @param best Current nearest hit payload.
		 */
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

		/**
		 * @brief Tests every triangle stored by a leaf node.
		 *
		 * @param bvh BVH to query.
		 * @param node Leaf node to test.
		 * @param ray Object-space ray.
		 * @param bestDistance Current nearest hit distance.
		 * @param best Current nearest hit payload.
		 */
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

		/**
		 * @brief Collects faces whose triangle bounds overlap query bounds.
		 *
		 * @param bvh BVH to query.
		 * @param nodeIndex Current node index.
		 * @param bounds Query bounds in object space.
		 * @param result Unique face handles collected so far.
		 */
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

		/**
		 * @brief Tests whether a node subtree overlaps query bounds.
		 *
		 * @param bvh BVH to query.
		 * @param nodeIndex Current node index.
		 * @param bounds Query bounds in object space.
		 * @return True when any triangle in the subtree overlaps.
		 */
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

		/**
		 * @brief Checks whether a face handle is already in a result list.
		 *
		 * @param handles Existing handle list.
		 * @param handle Face handle to find.
		 * @return True when the handle is present.
		 */
		[[nodiscard]] static bool contains(const std::vector<FaceHandle>& handles, FaceHandle handle) {
			return std::find(handles.begin(), handles.end(), handle) != handles.end();
		}
	};

}
