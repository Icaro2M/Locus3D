#include "kernel/geometry/queries/AdjacencyQuery.h"
#include "kernel/geometry/queries/BoundsQuery.h"
#include "kernel/geometry/queries/ProximityQuery.h"
#include "kernel/geometry/queries/RaycastQuery.h"
#include "kernel/geometry/queries/SelectionQuery.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyBuilder.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/math/Ray.h"

#include <glm/glm.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

    const char* hit_type_name(locus::kernel::geometry::LEMElementType type)
    {
        using namespace locus::kernel::geometry;

        switch (type) {
        case LEMElementType::Vertex:
            return "vertex";
        case LEMElementType::Edge:
            return "edge";
        case LEMElementType::Loop:
            return "loop";
        case LEMElementType::Face:
            return "face";
        }

        return "unknown";
    }

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

    void print_hit(const std::string& label, const locus::kernel::geometry::SelectionHit& hit)
    {
        std::cout << "  " << label << ": ";

        if (!hit.hit) {
            std::cout << "miss\n";
            return;
        }

        std::cout
            << hit_type_name(hit.type)
            << " distance="
            << hit.distance
            << " ";

        print_vec3("position=", hit.position);
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

    bool check_float(const std::string& label, float actual, float expected, float epsilon = 0.001f)
    {
        const bool passed = std::abs(actual - expected) <= epsilon;

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

    locus::kernel::geometry::LEM build_test_box()
    {
        using namespace locus::kernel::geometry;

        LEM mesh = TopologyBuilder::build_box(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f }
        );

        NormalBuilder::rebuild_face_normals(mesh);
        return mesh;
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

        LEM mesh = build_test_box();

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
        passed = check_float("selected min x", selectedBounds.min.x, -1.0f) && passed;
        passed = check_float("selected min y", selectedBounds.min.y, -1.0f) && passed;
        passed = check_float("selected min z", selectedBounds.min.z, -1.0f) && passed;
        passed = check_float("selected max x", selectedBounds.max.x, 1.0f) && passed;
        passed = check_float("selected max y", selectedBounds.max.y, 1.0f) && passed;
        passed = check_float("selected max z", selectedBounds.max.z, 1.0f) && passed;

        return passed;
    }

    bool test_proximity_query()
    {
        using namespace locus::kernel::geometry;

        std::cout << "\n=== Proximity Query ===\n";

        LEM mesh = build_test_box();

        const SelectionHit vertexHit = ProximityQuery::closest_vertex(
            mesh,
            glm::vec3{ -0.52f, -0.50f, -0.50f },
            0.1f
        );

        const SelectionHit edgeHit = ProximityQuery::closest_edge(
            mesh,
            glm::vec3{ 0.0f, -0.52f, -0.50f },
            0.1f
        );

        const SelectionHit faceHit = ProximityQuery::closest_face(
            mesh,
            glm::vec3{ 0.0f, 0.0f, -0.60f },
            0.2f
        );

        const SelectionHit elementHit = ProximityQuery::closest_element(
            mesh,
            glm::vec3{ -0.52f, -0.50f, -0.50f },
            0.1f
        );

        print_hit("closest vertex", vertexHit);
        print_hit("closest edge", edgeHit);
        print_hit("closest face", faceHit);
        print_hit("closest element", elementHit);

        bool passed = true;
        passed = check_bool("closest vertex hit", vertexHit.is_vertex()) && passed;
        passed = check_bool("closest edge hit", edgeHit.is_edge()) && passed;
        passed = check_bool("closest face hit", faceHit.is_face()) && passed;
        passed = check_bool("closest element hit", elementHit.hit) && passed;

        return passed;
    }

    bool test_raycast_query()
    {
        using namespace locus::kernel::geometry;

        std::cout << "\n=== Raycast Query ===\n";

        LEM mesh = build_test_box();

        const locus::kernel::math::Ray ray{
            glm::vec3{ 0.0f, 0.0f, -3.0f },
            glm::vec3{ 0.0f, 0.0f, 1.0f }
        };

        const SelectionHit faceHit = RaycastQuery::raycast_faces(mesh, ray);
        const SelectionHit edgeHit = RaycastQuery::raycast_edges(mesh, ray, 0.55f);
        const SelectionHit vertexHit = RaycastQuery::raycast_vertices(mesh, ray, 0.8f);
        const SelectionHit elementHit = RaycastQuery::raycast_element(mesh, ray, 0.8f, 0.55f);
        const locus::kernel::math::RayHit boundsHit = RaycastQuery::raycast_mesh_bounds(mesh, ray);

        print_hit("raycast face", faceHit);
        print_hit("raycast edge", edgeHit);
        print_hit("raycast vertex", vertexHit);
        print_hit("raycast element", elementHit);

        std::cout
            << "  bounds hit: "
            << (boundsHit.hit ? "sim" : "nao")
            << " distance="
            << boundsHit.distance
            << "\n";

        bool passed = true;
        passed = check_bool("face ray hit", faceHit.is_face()) && passed;
        passed = check_bool("edge ray hit", edgeHit.is_edge()) && passed;
        passed = check_bool("vertex ray hit", vertexHit.is_vertex()) && passed;
        passed = check_bool("element ray hit", elementHit.hit) && passed;
        passed = check_bool("bounds ray hit", boundsHit.hit) && passed;
        passed = check_float("bounds ray distance", boundsHit.distance, 2.5f) && passed;

        return passed;
    }

    bool test_selection_query()
    {
        using namespace locus::kernel::geometry;

        std::cout << "\n=== Selection Query ===\n";

        LEM mesh = build_test_box();

        SelectionQueryOptions faceOnly{};
        faceOnly.mask = SelectionElementMask::Face;

        SelectionQueryOptions allElements{};
        allElements.mask = SelectionElementMask::All;
        allElements.vertexRadius = 0.8f;
        allElements.edgeRadius = 0.55f;

        const locus::kernel::math::Ray ray{
            glm::vec3{ 0.0f, 0.0f, -3.0f },
            glm::vec3{ 0.0f, 0.0f, 1.0f }
        };

        const SelectionHit faceRayHit = SelectionQuery::pick_by_ray(mesh, ray, faceOnly);
        const SelectionHit allRayHit = SelectionQuery::pick_by_ray(mesh, ray, allElements);

        const SelectionHit pointHit = SelectionQuery::pick_by_point(
            mesh,
            glm::vec3{ -0.52f, -0.50f, -0.50f },
            allElements
        );

        print_hit("selection face ray", faceRayHit);
        print_hit("selection all ray", allRayHit);
        print_hit("selection point", pointHit);

        bool passed = true;
        passed = check_bool("face ray selection", faceRayHit.is_face()) && passed;
        passed = check_bool("all ray selection", allRayHit.hit) && passed;
        passed = check_bool("point selection", pointHit.hit) && passed;

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
    passed = test_proximity_query() && passed;
    passed = test_raycast_query() && passed;
    passed = test_selection_query() && passed;

    std::cout << "\n=== Resultado ===\n";
    std::cout << (passed ? "Todos os testes passaram.\n" : "Algum teste falhou.\n");

    return passed ? 0 : 1;
}