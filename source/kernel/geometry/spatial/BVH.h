#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/math/Bounds.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace locus::kernel::geometry {

	struct BVHTriangle {
		FaceHandle face{};
		glm::vec3 a{ 0.0f, 0.0f, 0.0f };
		glm::vec3 b{ 0.0f, 0.0f, 0.0f };
		glm::vec3 c{ 0.0f, 0.0f, 0.0f };
		glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
		math::Bounds bounds{};
	};

	struct BVHNode {
		static constexpr std::uint32_t InvalidNode = std::numeric_limits<std::uint32_t>::max();

		math::Bounds bounds{};
		std::uint32_t left = InvalidNode;
		std::uint32_t right = InvalidNode;
		std::uint32_t firstTriangle = 0;
		std::uint32_t triangleCount = 0;

		[[nodiscard]] bool is_leaf() const {
			return left == InvalidNode && right == InvalidNode;
		}
	};

	class BVH {
	public:
		void clear();

		[[nodiscard]] bool empty() const;
		[[nodiscard]] bool is_valid() const;

		[[nodiscard]] std::uint32_t root() const;
		[[nodiscard]] const math::Bounds& bounds() const;

		[[nodiscard]] std::size_t node_count() const;
		[[nodiscard]] std::size_t triangle_count() const;

		[[nodiscard]] const BVHNode& node(std::uint32_t index) const;
		[[nodiscard]] const BVHTriangle& triangle(std::uint32_t index) const;

		[[nodiscard]] const std::vector<BVHNode>& nodes() const;
		[[nodiscard]] const std::vector<BVHTriangle>& triangles() const;

	private:
		friend class BVHBuilder;

		std::vector<BVHNode> nodes_;
		std::vector<BVHTriangle> triangles_;
		std::uint32_t root_ = BVHNode::InvalidNode;
		math::Bounds bounds_ = math::Bounds::empty();
	};

}