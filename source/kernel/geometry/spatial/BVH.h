/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/math/Bounds.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace locus::kernel::geometry {

	/**
	 * @brief Triangle primitive stored in a bounding volume hierarchy.
	 */
	struct BVHTriangle {
		/**
		 * @brief Source face represented by this triangle.
		 */
		FaceHandle face{};
		/**
		 * @brief First triangle vertex in object space.
		 */
		glm::vec3 a{ 0.0f, 0.0f, 0.0f };
		/**
		 * @brief Second triangle vertex in object space.
		 */
		glm::vec3 b{ 0.0f, 0.0f, 0.0f };
		/**
		 * @brief Third triangle vertex in object space.
		 */
		glm::vec3 c{ 0.0f, 0.0f, 0.0f };
		/**
		 * @brief Source face normal used for hit reporting.
		 */
		glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
		/**
		 * @brief Axis-aligned bounds enclosing the triangle.
		 */
		math::Bounds bounds{};
	};

	/**
	 * @brief Node in a binary bounding volume hierarchy.
	 */
	struct BVHNode {
		/**
		 * @brief Sentinel index used when a child node does not exist.
		 */
		static constexpr std::uint32_t InvalidNode = std::numeric_limits<std::uint32_t>::max();

		/**
		 * @brief Bounds enclosing this node's child nodes or triangles.
		 */
		math::Bounds bounds{};
		/**
		 * @brief Left child node index, or InvalidNode for leaf nodes.
		 */
		std::uint32_t left = InvalidNode;
		/**
		 * @brief Right child node index, or InvalidNode for leaf nodes.
		 */
		std::uint32_t right = InvalidNode;
		/**
		 * @brief First triangle index for leaf nodes.
		 */
		std::uint32_t firstTriangle = 0;
		/**
		 * @brief Number of triangles stored by a leaf node.
		 */
		std::uint32_t triangleCount = 0;

		/**
		 * @brief Checks whether this node stores triangles directly.
		 *
		 * @return True when both child indices are InvalidNode.
		 */
		[[nodiscard]] bool is_leaf() const {
			return left == InvalidNode && right == InvalidNode;
		}
	};

	/**
	 * @brief Bounding volume hierarchy over visible mesh face triangles.
	 */
	class BVH {
	public:
		/**
		 * @brief Removes all nodes and triangles.
		 */
		void clear();

		/**
		 * @brief Checks whether the hierarchy contains no triangles.
		 *
		 * @return True when no triangle primitives are stored.
		 */
		[[nodiscard]] bool empty() const;
		/**
		 * @brief Checks whether the hierarchy has a valid root and bounds.
		 *
		 * @return True when the hierarchy can be queried.
		 */
		[[nodiscard]] bool is_valid() const;

		/**
		 * @brief Returns the root node index.
		 *
		 * @return Root node index, or BVHNode::InvalidNode for an empty hierarchy.
		 */
		[[nodiscard]] std::uint32_t root() const;
		/**
		 * @brief Returns the hierarchy bounds.
		 *
		 * @return Bounds enclosing all stored triangles.
		 */
		[[nodiscard]] const math::Bounds& bounds() const;

		/**
		 * @brief Returns the number of stored nodes.
		 *
		 * @return Node count.
		 */
		[[nodiscard]] std::size_t node_count() const;
		/**
		 * @brief Returns the number of stored triangle primitives.
		 *
		 * @return Triangle count.
		 */
		[[nodiscard]] std::size_t triangle_count() const;

		/**
		 * @brief Returns a node by index.
		 *
		 * @param index Node index.
		 * @return Read-only node reference.
		 */
		[[nodiscard]] const BVHNode& node(std::uint32_t index) const;
		/**
		 * @brief Returns a triangle primitive by index.
		 *
		 * @param index Triangle index.
		 * @return Read-only triangle reference.
		 */
		[[nodiscard]] const BVHTriangle& triangle(std::uint32_t index) const;

		/**
		 * @brief Returns all stored nodes.
		 *
		 * @return Read-only node storage.
		 */
		[[nodiscard]] const std::vector<BVHNode>& nodes() const;
		/**
		 * @brief Returns all stored triangle primitives.
		 *
		 * @return Read-only triangle storage.
		 */
		[[nodiscard]] const std::vector<BVHTriangle>& triangles() const;

	private:
		friend class BVHBuilder;

		std::vector<BVHNode> nodes_;
		std::vector<BVHTriangle> triangles_;
		std::uint32_t root_ = BVHNode::InvalidNode;
		math::Bounds bounds_ = math::Bounds::empty();
	};

}
