/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SpatialTestSuite.h"

#include "kernel/geometry/spatial/SpatialIndex.h"
#include "kernel/math/Bounds.h"
#include "kernel/math/Ray.h"

namespace locus::tests {

TestResult run_spatial_index_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::math;

    SpatialIndex emptyIndex;
    if (!emptyIndex.empty() ||
        emptyIndex.is_valid() ||
        emptyIndex.bvh().triangle_count() != 0) {
        return TestResult::fail("SpatialIndex should start empty and invalid");
    }

    LEM mesh;
    LEMEditor editor(mesh);
    const SpatialQuad quad = make_spatial_quad(editor);

    SpatialIndex index(mesh);
    if (index.empty() ||
        !index.is_valid() ||
        index.bvh().triangle_count() != 2) {
        return TestResult::fail("SpatialIndex constructor should build from visible mesh faces");
    }

    const Ray ray{ glm::vec3{ 0.0f, 0.0f, 2.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f } };
    const SelectionHit hit = index.raycast_faces(ray);
    if (!hit.is_face() ||
        hit.face != quad.face ||
        hit.distance < 1.9f ||
        hit.distance > 2.1f) {
        return TestResult::fail("SpatialIndex should raycast through its backing BVH");
    }

    const Bounds quadBounds = Bounds::from_min_max(
        glm::vec3{ -0.5f, -0.5f, -0.1f },
        glm::vec3{ 0.5f, 0.5f, 0.1f });
    const std::vector<FaceHandle> faces = index.query_bounds(quadBounds);
    if (faces.size() != 1 ||
        faces[0] != quad.face ||
        !index.intersects_bounds(quadBounds)) {
        return TestResult::fail("SpatialIndex should query bounds through its backing BVH");
    }

    editor.set_hidden(quad.face, true);
    index.rebuild(mesh);
    if (!index.empty() ||
        index.is_valid() ||
        index.raycast_faces(ray).hit ||
        index.intersects_bounds(quadBounds) ||
        !index.query_bounds(quadBounds).empty()) {
        return TestResult::fail("SpatialIndex rebuild should replace stale indexed data");
    }

    editor.set_hidden(quad.face, false);
    BVHBuildSettings settings;
    settings.maxLeafTriangles = 1;
    index.rebuild(mesh, settings);
    if (index.empty() ||
        !index.is_valid() ||
        index.bvh().node_count() < 3) {
        return TestResult::fail("SpatialIndex rebuild should pass BVH build settings through");
    }

    index.clear();
    if (!index.empty() ||
        index.is_valid() ||
        index.bvh().node_count() != 0 ||
        index.bvh().triangle_count() != 0) {
        return TestResult::fail("SpatialIndex clear should reset the backing BVH");
    }

    return TestResult::pass();
}

} // namespace locus::tests
