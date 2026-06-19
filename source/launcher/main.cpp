#include "kernel/geometry/queries/AdjacencyQuery.h"
#include "kernel/geometry/queries/BoundsQuery.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyBuilder.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <glm/glm.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

    void print_vec3(const std::string& label, const glm::vec3& value)
    {
        std::cout
            << label
            << "("
            << value.x
            << ", "
            << value.y
            << ", "
            << value.z
            << ")";
    }

    void print_bounds(const std::string& label, const locus::kernel::math::Bounds& bounds)
    {
        std::cout << label << ": ";

        if (!bounds.is_valid()) {
            std::cout << "invalid\n";
            return;
        }

        print_vec3("min=", bounds.min);
        std::cout << " ";
        print_vec3("max=", bounds.max);
        std::cout << " ";
        print_vec3("size=", bounds.size());
        std::cout << "\n";
    }

    bool check_value(const std::string& label, std::size_t actual, std::size_t expected)
    {
        const bool passed = actual == expected;

        std::cout
            << "  "
            << label
            << ": "
            << actual
            << " esperado "
            << expected
            << " -> "
            << (passed ? "OK" : "FALHOU")
            << "\n";

        return passed;
    }

    bool check_bool(const std::string& label, bool value)
    {
        std::cout
            << "  "
            << label
            << ": "
            << (value ? "OK" : "FALHOU")
            << "\n";

        return value;
    }

    void print_validation_report(const locus::kernel::geometry::TopologyValidationReport& report)
    {
        std::cout
            << "  validation valid: "
            << (report.valid() ? "sim" : "nao")
            << "\n";

        std::cout
            << "  issues: "
            << report.issues.size()
            << " | errors: "
            << report.error_count()
            << " | warnings: "
            << report.warning_count()
            << "\n";
    }

    bool test_quad()
    {
        using namespace locus::kernel::geometry;

        std::cout << "\n=== Quad ===\n";

        LEM mesh = TopologyBuilder::build_quad(
            glm::vec3{ -0.5f, 0.0f, -0.5f },
            glm::vec3{ 0.5f, 0.0f, -0.5f },
            glm::vec3{ 0.5f, 0.0f, 0.5f },
            glm::vec3{ -0.5f, 0.0f, 0.5f }
        );

        NormalBuilder::rebuild_face_normals(mesh);

        const TopologyValidationReport report = TopologyValidator::validate(mesh);
        const RenderMesh renderMesh = MeshTriangulator::triangulate(mesh);
        const locus::kernel::math::Bounds bounds = BoundsQuery::mesh_bounds(mesh);

        print_validation_report(report);
        print_bounds("  bounds", bounds);

        bool passed = true;
        passed = check_value("vertices", mesh.vertex_count(), 4) && passed;
        passed = check_value("edges", mesh.edge_count(), 4) && passed;
        passed = check_value("loops", mesh.loop_count(), 4) && passed;
        passed = check_value("faces", mesh.face_count(), 1) && passed;
        passed = check_value("triangles", renderMesh.triangle_count(), 2) && passed;
        passed = check_bool("topology valid", report.valid()) && passed;
        passed = check_bool("bounds valid", bounds.is_valid()) && passed;

        if (mesh.face_count() > 0) {
            const FaceHandle face(0);
            passed = check_value("face vertices", AdjacencyQuery::face_vertices(mesh, face).size(), 4) && passed;
            passed = check_value("face edges", AdjacencyQuery::face_edges(mesh, face).size(), 4) && passed;
            passed = check_value("face adjacent faces", AdjacencyQuery::adjacent_faces(mesh, face).size(), 0) && passed;
        }

        return passed;
    }

    bool test_box()
    {
        using namespace locus::kernel::geometry;

        std::cout << "\n=== Box ===\n";

        LEM mesh = TopologyBuilder::build_box(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f }
        );

        NormalBuilder::rebuild_face_normals(mesh);

        const TopologyValidationReport report = TopologyValidator::validate(mesh);
        const RenderMesh renderMesh = MeshTriangulator::triangulate(mesh);
        const locus::kernel::math::Bounds bounds = BoundsQuery::mesh_bounds(mesh);

        print_validation_report(report);
        print_bounds("  bounds", bounds);

        bool passed = true;
        passed = check_value("vertices", mesh.vertex_count(), 8) && passed;
        passed = check_value("edges", mesh.edge_count(), 12) && passed;
        passed = check_value("loops", mesh.loop_count(), 24) && passed;
        passed = check_value("faces", mesh.face_count(), 6) && passed;
        passed = check_value("triangles", renderMesh.triangle_count(), 12) && passed;
        passed = check_bool("topology valid", report.valid()) && passed;
        passed = check_bool("bounds valid", bounds.is_valid()) && passed;

        if (mesh.vertex_count() > 0) {
            const VertexHandle vertex(0);
            passed = check_value("vertex 0 adjacent vertices", AdjacencyQuery::adjacent_vertices(mesh, vertex).size(), 3) && passed;
            passed = check_value("vertex 0 edges", AdjacencyQuery::vertex_edges(mesh, vertex).size(), 3) && passed;
            passed = check_value("vertex 0 faces", AdjacencyQuery::vertex_faces(mesh, vertex).size(), 3) && passed;
        }

        if (mesh.edge_count() > 0) {
            const EdgeHandle edge(0);
            passed = check_value("edge 0 vertices", AdjacencyQuery::edge_vertices(mesh, edge).size(), 2) && passed;
            passed = check_value("edge 0 faces", AdjacencyQuery::edge_faces(mesh, edge).size(), 2) && passed;
            passed = check_bool("edge 0 manifold", AdjacencyQuery::is_manifold_edge(mesh, edge)) && passed;
        }

        if (mesh.face_count() > 0) {
            const FaceHandle face(0);
            passed = check_value("face 0 vertices", AdjacencyQuery::face_vertices(mesh, face).size(), 4) && passed;
            passed = check_value("face 0 edges", AdjacencyQuery::face_edges(mesh, face).size(), 4) && passed;
            passed = check_value("face 0 adjacent faces", AdjacencyQuery::adjacent_faces(mesh, face).size(), 4) && passed;
        }

        return passed;
    }

    bool test_selection_bounds()
    {
        using namespace locus::kernel::geometry;

        std::cout << "\n=== Selection Bounds ===\n";

        LEM mesh = TopologyBuilder::build_box(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 2.0f, 2.0f, 2.0f }
        );

        if (mesh.is_valid(VertexHandle(0))) {
            mesh.vertex(VertexHandle(0)).selected = true;
        }

        if (mesh.is_valid(VertexHandle(6))) {
            mesh.vertex(VertexHandle(6)).selected = true;
        }

        const locus::kernel::math::Bounds selectedBounds = BoundsQuery::selected_bounds(mesh);

        print_bounds("  selected bounds", selectedBounds);

        bool passed = true;
        passed = check_bool("selected bounds valid", selectedBounds.is_valid()) && passed;
        passed = check_bool("selected min x", selectedBounds.min.x == -1.0f) && passed;
        passed = check_bool("selected min y", selectedBounds.min.y == -1.0f) && passed;
        passed = check_bool("selected min z", selectedBounds.min.z == -1.0f) && passed;
        passed = check_bool("selected max x", selectedBounds.max.x == 1.0f) && passed;
        passed = check_bool("selected max y", selectedBounds.max.y == 1.0f) && passed;
        passed = check_bool("selected max z", selectedBounds.max.z == 1.0f) && passed;

        return passed;
    }

}

int main()
{
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Locus3D kernel consistency test\n";

    bool passed = true;

    passed = test_quad() && passed;
    passed = test_box() && passed;
    passed = test_selection_bounds() && passed;

    std::cout << "\n=== Resultado ===\n";
    std::cout << (passed ? "Todos os testes passaram.\n" : "Algum teste falhou.\n");

    return passed ? 0 : 1;
}