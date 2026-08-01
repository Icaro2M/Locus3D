/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/topology/FillHoleOp.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <vector>

namespace {

struct CycleFixture {
    std::vector<locus::kernel::geometry::VertexHandle> vertices{};
    std::vector<locus::kernel::geometry::EdgeHandle> edges{};
};

struct Counts {
    std::size_t vertices = 0u;
    std::size_t edges = 0u;
    std::size_t loops = 0u;
    std::size_t faces = 0u;
};

[[nodiscard]] Counts active_counts(
    const locus::kernel::geometry::LEM& mesh)
{
    return Counts{
        locus::kernel::geometry::TopologyTraversal::vertices(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::edges(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::loops(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::faces(mesh).size()
    };
}

[[nodiscard]] CycleFixture make_loose_cycle(
    locus::kernel::geometry::LEMEditor& editor,
    const std::vector<glm::vec3>& positions)
{
    CycleFixture fixture{};
    fixture.vertices.reserve(positions.size());
    fixture.edges.reserve(positions.size());

    for (const glm::vec3& position : positions) {
        fixture.vertices.push_back(editor.add_vertex(position));
    }

    for (std::size_t index = 0u; index < fixture.vertices.size(); ++index) {
        fixture.edges.push_back(
            editor.find_or_create_edge(
                fixture.vertices[index],
                fixture.vertices[(index + 1u) % fixture.vertices.size()]));
    }

    return fixture;
}

[[nodiscard]] CycleFixture make_regular_cycle(
    locus::kernel::geometry::LEMEditor& editor,
    std::size_t sides)
{
    std::vector<glm::vec3> positions{};
    positions.reserve(sides);

    for (std::size_t index = 0u; index < sides; ++index) {
        const float angle =
            6.28318530718f * static_cast<float>(index) /
            static_cast<float>(sides);
        positions.push_back(
            glm::vec3{
                glm::cos(angle),
                glm::sin(angle),
                0.0f });
    }

    return make_loose_cycle(editor, positions);
}

struct CubeHoleFixture {
    std::array<locus::kernel::geometry::VertexHandle, 8u> vertices{};
    std::vector<locus::kernel::geometry::EdgeHandle> holeEdges{};
};

[[nodiscard]] CubeHoleFixture make_cube_without_top(
    locus::kernel::geometry::LEMEditor& editor)
{
    using locus::kernel::geometry::EdgeHandle;
    using locus::kernel::geometry::VertexHandle;

    CubeHoleFixture fixture{};
    fixture.vertices = {
        editor.add_vertex(glm::vec3{ -1.0f, -1.0f, -1.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, -1.0f, -1.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, 1.0f, -1.0f }),
        editor.add_vertex(glm::vec3{ -1.0f, 1.0f, -1.0f }),
        editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 1.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 1.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 1.0f }),
        editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 1.0f })
    };

    const auto& v = fixture.vertices;
    (void)editor.add_face({ v[0], v[1], v[2], v[3] });
    (void)editor.add_face({ v[0], v[4], v[5], v[1] });
    (void)editor.add_face({ v[1], v[5], v[6], v[2] });
    (void)editor.add_face({ v[2], v[6], v[7], v[3] });
    (void)editor.add_face({ v[3], v[7], v[4], v[0] });

    fixture.holeEdges = {
        editor.find_or_create_edge(v[4], v[5]),
        editor.find_or_create_edge(v[5], v[6]),
        editor.find_or_create_edge(v[6], v[7]),
        editor.find_or_create_edge(v[7], v[4])
    };

    for (EdgeHandle edge : fixture.holeEdges) {
        if (!editor.mesh().is_valid(edge)) {
            return {};
        }
    }

    return fixture;
}

[[nodiscard]] locus::kernel::modeling::OperationContext make_context(
    locus::kernel::geometry::LEM& mesh,
    bool allowNonManifold = false)
{
    locus::kernel::modeling::OperationContext context{};
    context.mesh = &mesh;
    context.validateAfterExecute = true;
    context.rebuildNormals = true;
    context.allowNonManifold = allowNonManifold;
    return context;
}

[[nodiscard]] bool face_uses_vertices(
    const locus::kernel::geometry::LEM& mesh,
    locus::kernel::geometry::FaceHandle face,
    const std::vector<locus::kernel::geometry::VertexHandle>& expected)
{
    const std::vector<locus::kernel::geometry::VertexHandle> actual =
        locus::kernel::geometry::TopologyTraversal::face_vertices(
            mesh,
            face);

    if (actual.size() != expected.size()) {
        return false;
    }

    for (locus::kernel::geometry::VertexHandle vertex : expected) {
        bool found = false;
        for (locus::kernel::geometry::VertexHandle actualVertex : actual) {
            if (vertex == actualVertex) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool run_success_case(
    locus::kernel::geometry::LEM& mesh,
    locus::kernel::modeling::FillHoleOp operation,
    std::size_t expectedFaceVertices,
    const std::vector<locus::kernel::geometry::VertexHandle>& expectedVertices)
{
    using namespace locus::kernel::geometry;
    using namespace locus::kernel::modeling;

    const Counts before = active_counts(mesh);
    OperationContext context = make_context(mesh);

    const OperationResult result = operation.execute(context);

    if (!result.is_success() || !result.changed()) {
        return false;
    }

    const Counts after = active_counts(mesh);
    if (after.vertices != before.vertices ||
        after.edges != before.edges ||
        after.faces != before.faces + 1u ||
        after.loops != before.loops + expectedFaceVertices) {
        return false;
    }

    const std::vector<FaceHandle> faces = TopologyTraversal::faces(mesh);
    const FaceHandle newFace = faces.back();

    return TopologyTraversal::face_loops(mesh, newFace).size() ==
            expectedFaceVertices &&
        TopologyTraversal::face_edges(mesh, newFace).size() ==
            expectedFaceVertices &&
        face_uses_vertices(mesh, newFace, expectedVertices) &&
        TopologyValidator::validate(mesh).valid();
}

[[nodiscard]] bool operation_preserves_counts_on_failure(
    locus::kernel::geometry::LEM& mesh,
    locus::kernel::modeling::FillHoleOp operation)
{
    const Counts before = active_counts(mesh);
    locus::kernel::modeling::OperationContext context = make_context(mesh);
    const locus::kernel::modeling::OperationResult result =
        operation.execute(context);
    const Counts after = active_counts(mesh);

    return !result.is_success() &&
        before.vertices == after.vertices &&
        before.edges == after.edges &&
        before.loops == after.loops &&
        before.faces == after.faces &&
        locus::kernel::geometry::TopologyValidator::validate(mesh).valid();
}

} // namespace

namespace locus::tests {

TestResult run_fill_hole_operation_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::modeling;

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture triangle =
            make_regular_cycle(editor, 3u);

        if (!run_success_case(
                mesh,
                FillHoleOp::edges(triangle.edges),
                3u,
                triangle.vertices)) {
            return TestResult::fail(
                "FillHoleOp should create one triangular face from three boundary edges");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CubeHoleFixture cube =
            make_cube_without_top(editor);
        const Counts before = active_counts(mesh);

        if (!run_success_case(
                mesh,
                FillHoleOp::edges(
                    {
                        cube.holeEdges[2],
                        cube.holeEdges[0],
                        cube.holeEdges[3],
                        cube.holeEdges[1]
                    }),
                4u,
                {
                    cube.vertices[4],
                    cube.vertices[5],
                    cube.vertices[6],
                    cube.vertices[7]
                })) {
            return TestResult::fail(
                "FillHoleOp should close a cube hole with one quad face from unordered edges");
        }

        const Counts after = active_counts(mesh);
        if (after.vertices != before.vertices ||
            after.edges != before.edges ||
            after.faces != before.faces + 1u) {
            return TestResult::fail(
                "FillHoleOp quad should not add a center vertex or radial edges");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture ngon =
            make_regular_cycle(editor, 6u);

        if (!run_success_case(
                mesh,
                FillHoleOp::edges(
                    {
                        ngon.edges[3],
                        ngon.edges[0],
                        ngon.edges[5],
                        ngon.edges[1],
                        ngon.edges[4],
                        ngon.edges[2]
                    }),
                6u,
                ngon.vertices)) {
            return TestResult::fail(
                "FillHoleOp should create one n-gon face from a simple boundary cycle");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture quad =
            make_regular_cycle(editor, 4u);

        OperationContext context = make_context(mesh);
        FillHoleOp operation = FillHoleOp::edges(quad.edges);
        operation.set_flip_winding(true);

        const OperationResult result = operation.execute(context);
        const std::vector<FaceHandle> faces =
            TopologyTraversal::faces(mesh);

        if (!result.is_success() ||
            faces.size() != 1u ||
            TopologyTraversal::face_vertices(mesh, faces.front()).front() !=
                quad.vertices[3] ||
            TopologyTraversal::face_vertices(mesh, faces.front()).back() !=
                quad.vertices.front()) {
            return TestResult::fail(
                "FillHoleOp flip_winding should reverse the ordered cycle deterministically");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture cycle =
            make_regular_cycle(editor, 4u);

        editor.set_selected(cycle.edges[0], true);
        editor.set_selected(cycle.edges[1], true);
        editor.set_selected(cycle.edges[2], true);
        editor.set_selected(cycle.edges[3], true);

        if (!run_success_case(
                mesh,
                FillHoleOp::selected_boundary_edges(),
                4u,
                cycle.vertices)) {
            return TestResult::fail(
                "FillHoleOp should collect selected boundary edges when requested");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture cycle =
            make_regular_cycle(editor, 4u);

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges({ cycle.edges[0], cycle.edges[1] }))) {
            return TestResult::fail(
                "FillHoleOp should reject fewer than three edges without changes");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture cycle =
            make_regular_cycle(editor, 4u);

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges(
                    { cycle.edges[0], EdgeHandle{ 999u }, cycle.edges[2] }))) {
            return TestResult::fail(
                "FillHoleOp should reject invalid edge handles without changes");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const VertexHandle a =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle b =
            editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const VertexHandle c =
            editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
        const VertexHandle d =
            editor.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
        (void)editor.add_face({ a, b, c });
        (void)editor.add_face({ b, a, d });
        const EdgeHandle interior = mesh.find_edge(a, b);
        const EdgeHandle boundaryA = mesh.find_edge(b, c);
        const EdgeHandle boundaryB = mesh.find_edge(c, a);

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges({ interior, boundaryA, boundaryB }))) {
            return TestResult::fail(
                "FillHoleOp should reject non-boundary edges without changes");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const VertexHandle a =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle b =
            editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const VertexHandle c =
            editor.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f });
        const VertexHandle d =
            editor.add_vertex(glm::vec3{ 3.0f, 0.0f, 0.0f });
        const EdgeHandle ab = editor.find_or_create_edge(a, b);
        const EdgeHandle bc = editor.find_or_create_edge(b, c);
        const EdgeHandle cd = editor.find_or_create_edge(c, d);

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges({ ab, bc, cd }))) {
            return TestResult::fail(
                "FillHoleOp should reject open or colinear cycles without changes");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture first =
            make_loose_cycle(
                editor,
                {
                    glm::vec3{ 0.0f, 0.0f, 0.0f },
                    glm::vec3{ 1.0f, 0.0f, 0.0f },
                    glm::vec3{ 0.0f, 1.0f, 0.0f }
                });
        const CycleFixture second =
            make_loose_cycle(
                editor,
                {
                    glm::vec3{ 3.0f, 0.0f, 0.0f },
                    glm::vec3{ 4.0f, 0.0f, 0.0f },
                    glm::vec3{ 3.0f, 1.0f, 0.0f }
                });

        std::vector<EdgeHandle> disconnected = first.edges;
        disconnected.insert(
            disconnected.end(),
            second.edges.begin(),
            second.edges.end());

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges(disconnected))) {
            return TestResult::fail(
                "FillHoleOp should reject disconnected cycles without changes");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture triangle =
            make_regular_cycle(editor, 3u);
        const VertexHandle extra =
            editor.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f });
        const EdgeHandle branch =
            editor.find_or_create_edge(triangle.vertices[0], extra);
        std::vector<EdgeHandle> branched = triangle.edges;
        branched.push_back(branch);

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges(branched))) {
            return TestResult::fail(
                "FillHoleOp should reject branched edge sets without changes");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture line =
            make_loose_cycle(
                editor,
                {
                    glm::vec3{ 0.0f, 0.0f, 0.0f },
                    glm::vec3{ 1.0f, 0.0f, 0.0f },
                    glm::vec3{ 2.0f, 0.0f, 0.0f }
                });

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges(line.edges))) {
            return TestResult::fail(
                "FillHoleOp should reject degenerate colinear cycles without changes");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const CycleFixture triangle =
            make_regular_cycle(editor, 3u);
        (void)editor.add_face(triangle.vertices);
        (void)editor.add_face(
            {
                triangle.vertices[0],
                triangle.vertices[2],
                triangle.vertices[1]
            });

        if (!operation_preserves_counts_on_failure(
                mesh,
                FillHoleOp::edges(triangle.edges))) {
            return TestResult::fail(
                "FillHoleOp should reject non-manifold fills without changes");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
