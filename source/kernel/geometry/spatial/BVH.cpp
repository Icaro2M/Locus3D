/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/spatial/BVH.h"

#include <cassert>

namespace locus::kernel::geometry {

	void BVH::clear() {
		nodes_.clear();
		triangles_.clear();
		root_ = BVHNode::InvalidNode;
		bounds_ = math::Bounds::empty();
	}

	bool BVH::empty() const {
		return triangles_.empty();
	}

	bool BVH::is_valid() const {
		return root_ != BVHNode::InvalidNode && root_ < nodes_.size() && bounds_.is_valid();
	}

	std::uint32_t BVH::root() const {
		return root_;
	}

	const math::Bounds& BVH::bounds() const {
		return bounds_;
	}

	std::size_t BVH::node_count() const {
		return nodes_.size();
	}

	std::size_t BVH::triangle_count() const {
		return triangles_.size();
	}

	const BVHNode& BVH::node(std::uint32_t index) const {
		assert(index < nodes_.size());
		return nodes_[index];
	}

	const BVHTriangle& BVH::triangle(std::uint32_t index) const {
		assert(index < triangles_.size());
		return triangles_[index];
	}

	const std::vector<BVHNode>& BVH::nodes() const {
		return nodes_;
	}

	const std::vector<BVHTriangle>& BVH::triangles() const {
		return triangles_;
	}

}
