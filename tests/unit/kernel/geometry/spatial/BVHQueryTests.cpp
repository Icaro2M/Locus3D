/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SpatialTestSuite.h"

#include "kernel/geometry/spatial/BVHBuilder.h"
#include "kernel/geometry/spatial/BVHQuery.h"
#include "kernel/math/Bounds.h"
#include "kernel/math/Ray.h"

#include <algorithm>
#include <vector>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    const float diff = lhs - rhs;
    return diff >= -epsilon && diff <= epsilon;
}

[[nodiscard]] bool contains_face(
    const std::vector<locus::kernel::geometry::FaceHandle>& faces,
    locus::kernel::geometry::FaceHandle face)
{
    return std::find(faces.begin(), faces.end(), face) != faces.end();
}

} // namespace

namespace locus::tests {

TestResult run_bvh_query_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::math;

    BVH invalidBvh;
    const Ray defaultRay{ glm::vec3{ 0.0f, 0.0f, 1.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f } };
    if (BVHQuery::raycast_faces(invalidBvh, defaultRay).hit ||
        BVHQuery::intersects_bounds(invalidBvh, Bounds::from_center_size(glm::vec3{ 0.0f }, glm::vec3{ 1.0f })) ||
        !BVHQuery::query_bounds(invalidBvh, Bounds::from_center_size(glm::vec3{ 0.0f }, glm::vec3{ 1.0f })).empty()) {
        return TestResult::fail("BVHQuery should return misses for invalid BVHs");
    }

    LEM mesh;
    LEMEditor editor(mesh);
    const SpatialQuad nearQuad = make_spatial_quad(editor);
    const SpatialQuad farQuad = make_spatial_quad(editor, glm::vec3{ 4.0f, 0.0f, 0.0f });

    BVHBuildSettings settings;
    settings.maxLeafTriangles = 1;
    const BVH bvh = BVHBuilder::build(mesh, settings);

    const SelectionHit centerHit = BVHQuery::raycast_faces(bvh, defaultRay);
    if (!centerHit.is_face() ||
        centerHit.face != nearQuad.face ||
        !near(centerHit.distance, 1.0f) ||
        !near(centerHit.position.x, 0.0f) ||
        !near(centerHit.position.y, 0.0f) ||
        !near(centerHit.position.z, 0.0f)) {
        return TestResult::fail("BVHQuery raycast should return the nearest hit face");
    }

    if (BVHQuery::raycast_faces(bvh, defaultRay, 0.5f).hit) {
        return TestResult::fail("BVHQuery raycast should honor maxDistance");
    }

    const Ray missRay{ glm::vec3{ 0.0f, 0.0f, 1.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f } };
    if (BVHQuery::raycast_faces(bvh, missRay).hit) {
        return TestResult::fail("BVHQuery raycast should miss rays that do not intersect triangles");
    }

    const Bounds nearBounds = Bounds::from_min_max(
        glm::vec3{ -0.25f, -0.25f, -0.25f },
        glm::vec3{ 0.25f, 0.25f, 0.25f });
    const std::vector<FaceHandle> nearFaces = BVHQuery::query_bounds(bvh, nearBounds);
    if (nearFaces.size() != 1 ||
        !contains_face(nearFaces, nearQuad.face) ||
        !BVHQuery::intersects_bounds(bvh, nearBounds)) {
        return TestResult::fail("BVHQuery bounds query should return unique overlapping faces");
    }

    const Bounds farBounds = Bounds::from_min_max(
        glm::vec3{ 3.75f, -0.25f, -0.25f },
        glm::vec3{ 4.25f, 0.25f, 0.25f });
    const std::vector<FaceHandle> farFaces = BVHQuery::query_bounds(bvh, farBounds);
    if (farFaces.size() != 1 ||
        !contains_face(farFaces, farQuad.face) ||
        !BVHQuery::intersects_bounds(bvh, farBounds)) {
        return TestResult::fail("BVHQuery bounds query should traverse split child nodes");
    }

    const Bounds bothBounds = Bounds::from_min_max(
        glm::vec3{ -1.5f, -1.5f, -0.25f },
        glm::vec3{ 5.5f, 1.5f, 0.25f });
    const std::vector<FaceHandle> bothFaces = BVHQuery::query_bounds(bvh, bothBounds);
    if (bothFaces.size() != 2 ||
        !contains_face(bothFaces, nearQuad.face) ||
        !contains_face(bothFaces, farQuad.face)) {
        return TestResult::fail("BVHQuery bounds query should collect multiple overlapping faces");
    }

    const Bounds emptyBounds = Bounds::empty();
    if (BVHQuery::intersects_bounds(bvh, emptyBounds) ||
        !BVHQuery::query_bounds(bvh, emptyBounds).empty()) {
        return TestResult::fail("BVHQuery should reject invalid query bounds");
    }

    const Bounds missBounds = Bounds::from_center_size(
        glm::vec3{ 20.0f, 20.0f, 20.0f },
        glm::vec3{ 1.0f, 1.0f, 1.0f });
    if (BVHQuery::intersects_bounds(bvh, missBounds) ||
        !BVHQuery::query_bounds(bvh, missBounds).empty()) {
        return TestResult::fail("BVHQuery bounds query should miss non-overlapping bounds");
    }

    return TestResult::pass();
}

} // namespace locus::tests
