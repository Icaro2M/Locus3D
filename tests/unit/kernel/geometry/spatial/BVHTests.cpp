/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SpatialTestSuite.h"

#include "kernel/geometry/spatial/BVHBuilder.h"

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    const float diff = lhs - rhs;
    return diff >= -epsilon && diff <= epsilon;
}

} // namespace

namespace locus::tests {

TestResult run_bvh_tests()
{
    using namespace kernel::geometry;

    BVH emptyBvh;
    if (!emptyBvh.empty() ||
        emptyBvh.is_valid() ||
        emptyBvh.root() != BVHNode::InvalidNode ||
        emptyBvh.node_count() != 0 ||
        emptyBvh.triangle_count() != 0 ||
        emptyBvh.bounds().is_valid()) {
        return TestResult::fail("default BVH should start empty and invalid");
    }

    LEM emptyMesh;
    const BVH builtEmpty = BVHBuilder::build(emptyMesh);
    if (!builtEmpty.empty() ||
        builtEmpty.is_valid() ||
        builtEmpty.root() != BVHNode::InvalidNode ||
        builtEmpty.node_count() != 0 ||
        builtEmpty.triangle_count() != 0) {
        return TestResult::fail("BVHBuilder should leave an empty mesh unindexed");
    }

    LEM mesh;
    LEMEditor editor(mesh);
    const SpatialQuad quad = make_spatial_quad(editor);

    BVHBuildSettings settings;
    settings.maxLeafTriangles = 1;
    const BVH bvh = BVHBuilder::build(mesh, settings);

    if (bvh.empty() ||
        !bvh.is_valid() ||
        bvh.root() == BVHNode::InvalidNode ||
        bvh.triangle_count() != 2 ||
        bvh.node_count() < 3) {
        return TestResult::fail("BVHBuilder should triangulate a visible quad and split with small leaves");
    }

    const BVHNode& root = bvh.node(bvh.root());
    if (root.is_leaf() ||
        root.left == BVHNode::InvalidNode ||
        root.right == BVHNode::InvalidNode ||
        root.triangleCount != 0) {
        return TestResult::fail("BVH root should reference child nodes after splitting");
    }

    if (!near(bvh.bounds().min.x, -1.0f) ||
        !near(bvh.bounds().min.y, -1.0f) ||
        !near(bvh.bounds().min.z, 0.0f) ||
        !near(bvh.bounds().max.x, 1.0f) ||
        !near(bvh.bounds().max.y, 1.0f) ||
        !near(bvh.bounds().max.z, 0.0f)) {
        return TestResult::fail("BVH bounds should enclose the indexed quad");
    }

    for (const BVHTriangle& triangle : bvh.triangles()) {
        if (triangle.face != quad.face ||
            !triangle.bounds.is_valid() ||
            !near(triangle.normal.z, 1.0f)) {
            return TestResult::fail("BVH triangles should preserve face handle, bounds and normal");
        }
    }

    BVHBuildSettings sanitizedSettings;
    sanitizedSettings.maxLeafTriangles = 0;
    sanitizedSettings.maxDepth = 0;
    const BVH sanitizedBvh = BVHBuilder::build(mesh, sanitizedSettings);
    if (!sanitizedBvh.is_valid() ||
        sanitizedBvh.triangle_count() != 2 ||
        sanitizedBvh.node_count() == 0) {
        return TestResult::fail("BVHBuilder should sanitize zero construction limits");
    }

    editor.set_hidden(quad.face, true);
    BVH hiddenBvh;
    BVHBuilder::build_into(mesh, hiddenBvh);
    if (!hiddenBvh.empty() ||
        hiddenBvh.is_valid() ||
        hiddenBvh.triangle_count() != 0) {
        return TestResult::fail("BVHBuilder should ignore hidden faces");
    }

    hiddenBvh.clear();
    if (!hiddenBvh.empty() ||
        hiddenBvh.is_valid() ||
        hiddenBvh.root() != BVHNode::InvalidNode ||
        hiddenBvh.node_count() != 0 ||
        hiddenBvh.triangle_count() != 0) {
        return TestResult::fail("BVH::clear should reset all storage and root state");
    }

    return TestResult::pass();
}

} // namespace locus::tests
