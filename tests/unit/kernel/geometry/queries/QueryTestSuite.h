/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file QueryTestSuite.h
 * @brief Declarations and small fixtures for geometry query unit test groups.
 */

#include "common/TestResult.h"

#include "kernel/geometry/mesh/LEM.h"

#include <glm/vec3.hpp>

namespace locus::tests {

struct QueryMesh {
    kernel::geometry::LEM mesh;

    kernel::geometry::VertexHandle v0;
    kernel::geometry::VertexHandle v1;
    kernel::geometry::VertexHandle v2;
    kernel::geometry::VertexHandle v3;
    kernel::geometry::VertexHandle v4;
    kernel::geometry::VertexHandle v5;
    kernel::geometry::VertexHandle looseVertex;
    kernel::geometry::VertexHandle looseEdgeA;
    kernel::geometry::VertexHandle looseEdgeB;

    kernel::geometry::FaceHandle leftFace;
    kernel::geometry::FaceHandle rightFace;
    kernel::geometry::EdgeHandle sharedEdge;
    kernel::geometry::EdgeHandle boundaryEdge;
    kernel::geometry::EdgeHandle looseEdge;
};

[[nodiscard]] inline QueryMesh make_query_mesh()
{
    QueryMesh fixture{};
    fixture.v0 = fixture.mesh.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    fixture.v1 = fixture.mesh.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
    fixture.v2 = fixture.mesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    fixture.v3 = fixture.mesh.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });
    fixture.v4 = fixture.mesh.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    fixture.v5 = fixture.mesh.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });

    fixture.leftFace = fixture.mesh.add_face({ fixture.v0, fixture.v1, fixture.v2, fixture.v3 });
    fixture.rightFace = fixture.mesh.add_face({ fixture.v1, fixture.v4, fixture.v5, fixture.v2 });
    fixture.sharedEdge = fixture.mesh.find_edge(fixture.v1, fixture.v2);
    fixture.boundaryEdge = fixture.mesh.find_edge(fixture.v0, fixture.v1);

    fixture.looseVertex = fixture.mesh.add_vertex(glm::vec3{ 4.0f, 0.0f, 0.0f });
    fixture.looseEdgeA = fixture.mesh.add_vertex(glm::vec3{ 4.0f, 1.0f, 0.0f });
    fixture.looseEdgeB = fixture.mesh.add_vertex(glm::vec3{ 5.0f, 1.0f, 0.0f });
    fixture.looseEdge = fixture.mesh.find_or_create_edge(fixture.looseEdgeA, fixture.looseEdgeB);

    return fixture;
}

[[nodiscard]] TestResult run_selection_hit_tests();
[[nodiscard]] TestResult run_adjacency_query_tests();
[[nodiscard]] TestResult run_bounds_query_tests();
[[nodiscard]] TestResult run_picking_query_tests();

} // namespace locus::tests
