/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/spatial/BVH.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/math/GeometryMath.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace locus::kernel::geometry {

	/**
	 * @brief Settings used when constructing a BVH.
	 */
	struct BVHBuildSettings {
		/**
		 * @brief Maximum number of triangles stored in a leaf node.
		 */
		std::uint32_t maxLeafTriangles = 4;
		/**
		 * @brief Maximum recursive depth allowed during BVH construction.
		 */
		std::uint32_t maxDepth = 32;
	};

	/**
	 * @brief Builds a BVH from visible LEM face triangles.
	 */
	class BVHBuilder {
	public:
		/**
		 * @brief Creates a new BVH from a mesh.
		 *
		 * @param mesh Mesh whose visible faces are indexed.
		 * @param settings BVH construction settings.
		 * @return Built BVH, or an empty BVH when no triangles are available.
		 */
		[[nodiscard]] static BVH build(const LEM& mesh, const BVHBuildSettings& settings = {}) {
			BVH bvh;
			build_into(mesh, bvh, settings);
			return bvh;
		}

		/**
		 * @brief Rebuilds an output BVH from a mesh.
		 *
		 * @param mesh Mesh whose visible faces are indexed.
		 * @param output BVH object that receives the built hierarchy.
		 * @param settings BVH construction settings.
		 */
		static void build_into(const LEM& mesh, BVH& output, const BVHBuildSettings& settings = {}) {
			output.clear();
			collect_triangles(mesh, output.triangles_);

			if (output.triangles_.empty()) {
				return;
			}

			output.bounds_ = compute_bounds(output.triangles_, 0, static_cast<std::uint32_t>(output.triangles_.size()));
			output.root_ = build_node(output, 0, static_cast<std::uint32_t>(output.triangles_.size()), 0, sanitize(settings));
		}

	private:
		/**
		 * @brief Ensures build settings use non-zero construction limits.
		 *
		 * @param settings Requested build settings.
		 * @return Sanitized build settings.
		 */
		static BVHBuildSettings sanitize(BVHBuildSettings settings) {
			if (settings.maxLeafTriangles == 0) {
				settings.maxLeafTriangles = 1;
			}

			if (settings.maxDepth == 0) {
				settings.maxDepth = 1;
			}

			return settings;
		}

		/**
		 * @brief Collects non-degenerate visible face triangles from a mesh.
		 *
		 * Polygon faces are converted to triangle fans using the first face vertex
		 * as the fan anchor.
		 *
		 * @param mesh Mesh whose faces are triangulated.
		 * @param output Triangle list that receives collected primitives.
		 */
		static void collect_triangles(const LEM& mesh, std::vector<BVHTriangle>& output) {
			for (FaceHandle faceHandle : TopologyTraversal::faces(mesh)) {
				if (!mesh.is_valid(faceHandle)) {
					continue;
				}

				const Face& face = mesh.face(faceHandle);
				if (face.hidden) {
					continue;
				}

				const std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, faceHandle);
				if (vertices.size() < 3) {
					continue;
				}

				for (std::size_t i = 1; i + 1 < vertices.size(); ++i) {
					if (!mesh.is_valid(vertices[0]) || !mesh.is_valid(vertices[i]) || !mesh.is_valid(vertices[i + 1])) {
						continue;
					}

					const glm::vec3 a = mesh.vertex(vertices[0]).position;
					const glm::vec3 b = mesh.vertex(vertices[i]).position;
					const glm::vec3 c = mesh.vertex(vertices[i + 1]).position;

					if (math::triangle_area(a, b, c) <= math::Epsilon) {
						continue;
					}

					BVHTriangle triangle;
					triangle.face = faceHandle;
					triangle.a = a;
					triangle.b = b;
					triangle.c = c;
					triangle.normal = face.normal;
					triangle.bounds = triangle_bounds(a, b, c);

					output.push_back(triangle);
				}
			}
		}

		/**
		 * @brief Computes bounds for one triangle.
		 *
		 * @param a First triangle vertex.
		 * @param b Second triangle vertex.
		 * @param c Third triangle vertex.
		 * @return Bounds enclosing the triangle.
		 */
		[[nodiscard]] static math::Bounds triangle_bounds(
			const glm::vec3& a,
			const glm::vec3& b,
			const glm::vec3& c
		) {
			math::Bounds bounds = math::Bounds::empty();
			bounds.expand(a);
			bounds.expand(b);
			bounds.expand(c);
			return bounds;
		}

		/**
		 * @brief Computes a triangle centroid.
		 *
		 * @param triangle Triangle primitive.
		 * @return Object-space centroid.
		 */
		[[nodiscard]] static glm::vec3 triangle_centroid(const BVHTriangle& triangle) {
			return (triangle.a + triangle.b + triangle.c) / 3.0f;
		}

		/**
		 * @brief Computes bounds for a contiguous triangle range.
		 *
		 * @param triangles Triangle storage.
		 * @param first First triangle index.
		 * @param count Number of triangles in the range.
		 * @return Bounds enclosing the triangle range.
		 */
		[[nodiscard]] static math::Bounds compute_bounds(
			const std::vector<BVHTriangle>& triangles,
			std::uint32_t first,
			std::uint32_t count
		) {
			math::Bounds bounds = math::Bounds::empty();

			for (std::uint32_t i = 0; i < count; ++i) {
				bounds.expand(triangles[first + i].bounds);
			}

			return bounds;
		}

		/**
		 * @brief Computes centroid bounds for a contiguous triangle range.
		 *
		 * @param triangles Triangle storage.
		 * @param first First triangle index.
		 * @param count Number of triangles in the range.
		 * @return Bounds enclosing triangle centroids.
		 */
		[[nodiscard]] static math::Bounds compute_centroid_bounds(
			const std::vector<BVHTriangle>& triangles,
			std::uint32_t first,
			std::uint32_t count
		) {
			math::Bounds bounds = math::Bounds::empty();

			for (std::uint32_t i = 0; i < count; ++i) {
				bounds.expand(triangle_centroid(triangles[first + i]));
			}

			return bounds;
		}

		/**
		 * @brief Selects the largest axis of a size vector.
		 *
		 * @param size Bounds size vector.
		 * @return Axis index: 0 for X, 1 for Y, 2 for Z.
		 */
		[[nodiscard]] static int largest_axis(const glm::vec3& size) {
			if (size.x >= size.y && size.x >= size.z) {
				return 0;
			}

			if (size.y >= size.x && size.y >= size.z) {
				return 1;
			}

			return 2;
		}

		/**
		 * @brief Recursively builds a BVH node over a triangle range.
		 *
		 * Splits along the largest centroid-bounds axis using nth_element to
		 * partition triangles around the median centroid.
		 *
		 * @param bvh BVH being built.
		 * @param first First triangle index in the node range.
		 * @param count Number of triangles in the node range.
		 * @param depth Current recursive depth.
		 * @param settings Sanitized build settings.
		 * @return Index of the created node.
		 */
		[[nodiscard]] static std::uint32_t build_node(
			BVH& bvh,
			std::uint32_t first,
			std::uint32_t count,
			std::uint32_t depth,
			const BVHBuildSettings& settings
		) {
			const std::uint32_t nodeIndex = static_cast<std::uint32_t>(bvh.nodes_.size());

			BVHNode node;
			node.bounds = compute_bounds(bvh.triangles_, first, count);
			node.firstTriangle = first;
			node.triangleCount = count;

			bvh.nodes_.push_back(node);

			if (count <= settings.maxLeafTriangles || depth >= settings.maxDepth) {
				return nodeIndex;
			}

			const math::Bounds centroidBounds = compute_centroid_bounds(bvh.triangles_, first, count);
			const glm::vec3 centroidSize = centroidBounds.size();

			if (!centroidBounds.is_valid() || glm::length(centroidSize) <= math::Epsilon) {
				return nodeIndex;
			}

			const int axis = largest_axis(centroidSize);
			const std::uint32_t middle = first + count / 2;

			auto begin = bvh.triangles_.begin() + static_cast<std::ptrdiff_t>(first);
			auto mid = bvh.triangles_.begin() + static_cast<std::ptrdiff_t>(middle);
			auto end = bvh.triangles_.begin() + static_cast<std::ptrdiff_t>(first + count);

			std::nth_element(
				begin,
				mid,
				end,
				[axis](const BVHTriangle& lhs, const BVHTriangle& rhs) {
					return triangle_centroid(lhs)[axis] < triangle_centroid(rhs)[axis];
				}
			);

			const std::uint32_t leftCount = middle - first;
			const std::uint32_t rightCount = count - leftCount;

			if (leftCount == 0 || rightCount == 0) {
				return nodeIndex;
			}

			const std::uint32_t left = build_node(bvh, first, leftCount, depth + 1, settings);
			const std::uint32_t right = build_node(bvh, middle, rightCount, depth + 1, settings);

			BVHNode& storedNode = bvh.nodes_[nodeIndex];
			storedNode.left = left;
			storedNode.right = right;
			storedNode.triangleCount = 0;

			return nodeIndex;
		}
	};

}
