/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TopologyTestSuite.h"

#include "kernel/geometry/topology/TopologyBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <algorithm>

namespace {

[[nodiscard]] bool has_issue(
    const locus::kernel::geometry::TopologyValidationReport& report,
    locus::kernel::geometry::TopologyIssueCode code)
{
    return std::find_if(
        report.issues.begin(),
        report.issues.end(),
        [code](const locus::kernel::geometry::TopologyIssue& issue) {
            return issue.code == code;
        }) != report.issues.end();
}

} // namespace

namespace locus::tests {

TestResult run_topology_validator_tests()
{
    using namespace kernel::geometry;

    const LEM validBox = TopologyBuilder::build_box();
    const TopologyValidationReport validReport = TopologyValidator::validate(validBox);
    if (!validReport.valid() ||
        validReport.has_issues() ||
        validReport.error_count() != 0 ||
        validReport.warning_count() != 0) {
        return TestResult::fail("TopologyValidator should accept valid topology without diagnostics");
    }

    TopologyValidationReport warningOnly;
    warningOnly.issues.push_back(TopologyIssue{
        TopologyIssueSeverity::Warning,
        TopologyIssueCode::NonManifoldEdge,
        LEMElementType::Edge,
        {},
        "warning only"
    });
    if (!warningOnly.valid() ||
        !warningOnly.has_issues() ||
        warningOnly.error_count() != 0 ||
        warningOnly.warning_count() != 1) {
        return TestResult::fail("TopologyValidationReport should treat warning-only reports as valid");
    }

    TwoQuadTopology invalidEdgeFixture = make_two_quad_topology();
    invalidEdgeFixture.mesh.edge(invalidEdgeFixture.looseEdge).vertexB = VertexHandle{};
    const TopologyValidationReport invalidEdgeReport =
        TopologyValidator::validate(invalidEdgeFixture.mesh);
    if (invalidEdgeReport.valid() ||
        !has_issue(invalidEdgeReport, TopologyIssueCode::InvalidVertexReference)) {
        return TestResult::fail("TopologyValidator should report edges that reference invalid vertices");
    }

    TwoQuadTopology degenerateEdgeFixture = make_two_quad_topology();
    degenerateEdgeFixture.mesh.edge(degenerateEdgeFixture.looseEdge).vertexB =
        degenerateEdgeFixture.mesh.edge(degenerateEdgeFixture.looseEdge).vertexA;
    const TopologyValidationReport degenerateEdgeReport =
        TopologyValidator::validate(degenerateEdgeFixture.mesh);
    if (degenerateEdgeReport.valid() ||
        !has_issue(degenerateEdgeReport, TopologyIssueCode::DegenerateEdge)) {
        return TestResult::fail("TopologyValidator should report degenerate edges");
    }

    TwoQuadTopology invalidFaceFixture = make_two_quad_topology();
    invalidFaceFixture.mesh.face(invalidFaceFixture.leftFace).loop = LoopHandle{};
    const TopologyValidationReport invalidFaceReport =
        TopologyValidator::validate(invalidFaceFixture.mesh);
    if (invalidFaceReport.valid() ||
        !has_issue(invalidFaceReport, TopologyIssueCode::InvalidLoopReference)) {
        return TestResult::fail("TopologyValidator should report faces with invalid boundary loops");
    }

    TwoQuadTopology brokenFaceFixture = make_two_quad_topology();
    const std::vector<LoopHandle> faceLoops =
        TopologyTraversal::face_loops(brokenFaceFixture.mesh, brokenFaceFixture.leftFace);
    brokenFaceFixture.mesh.loop(faceLoops[1]).previous = faceLoops[1];
    const TopologyValidationReport brokenFaceReport =
        TopologyValidator::validate(brokenFaceFixture.mesh);
    if (brokenFaceReport.valid() ||
        !has_issue(brokenFaceReport, TopologyIssueCode::BrokenFaceCycle)) {
        return TestResult::fail("TopologyValidator should report inconsistent face cycle links");
    }

    TwoQuadTopology brokenRadialFixture = make_two_quad_topology();
    const std::vector<LoopHandle> radialLoops =
        TopologyTraversal::edge_loops(brokenRadialFixture.mesh, brokenRadialFixture.sharedEdge);
    brokenRadialFixture.mesh.loop(radialLoops[1]).radialPrevious = radialLoops[1];
    const TopologyValidationReport brokenRadialReport =
        TopologyValidator::validate(brokenRadialFixture.mesh);
    if (brokenRadialReport.valid() ||
        !has_issue(brokenRadialReport, TopologyIssueCode::BrokenRadialCycle)) {
        return TestResult::fail("TopologyValidator should report inconsistent radial cycle links");
    }

    TwoQuadTopology loopMismatchFixture = make_two_quad_topology();
    const std::vector<LoopHandle> mismatchLoops =
        TopologyTraversal::face_loops(loopMismatchFixture.mesh, loopMismatchFixture.leftFace);
    loopMismatchFixture.mesh.loop(mismatchLoops[0]).face = loopMismatchFixture.rightFace;
    loopMismatchFixture.mesh.loop(mismatchLoops[0]).vertex = loopMismatchFixture.v5;
    const TopologyValidationReport loopMismatchReport =
        TopologyValidator::validate(loopMismatchFixture.mesh);
    if (loopMismatchReport.valid() ||
        !has_issue(loopMismatchReport, TopologyIssueCode::LoopFaceMismatch) ||
        !has_issue(loopMismatchReport, TopologyIssueCode::EdgeEndpointMismatch)) {
        return TestResult::fail("TopologyValidator should report loop face and edge endpoint mismatches");
    }

    LEM nonManifoldMesh;
    const VertexHandle v0 = nonManifoldMesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = nonManifoldMesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    const VertexHandle v2 = nonManifoldMesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    const VertexHandle v3 = nonManifoldMesh.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
    const VertexHandle v4 = nonManifoldMesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 1.0f });
    nonManifoldMesh.add_face({ v0, v1, v2 });
    nonManifoldMesh.add_face({ v1, v0, v3 });
    nonManifoldMesh.add_face({ v0, v1, v4 });
    const TopologyValidationReport nonManifoldReport =
        TopologyValidator::validate(nonManifoldMesh);
    if (nonManifoldReport.valid() ||
        !has_issue(nonManifoldReport, TopologyIssueCode::NonManifoldEdge)) {
        return TestResult::fail("TopologyValidator should report edges with more than two radial loops");
    }

    return TestResult::pass();
}

} // namespace locus::tests
