#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/topology/FillHoleOp.h"

#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace geometry = locus::kernel::geometry;
namespace modeling = locus::kernel::modeling;

namespace {

    struct TestStats {
        int passed = 0;
        int failed = 0;
    };

    void check(TestStats& stats, bool condition, const std::string& message) {
        if (condition) {
            ++stats.passed;
            std::cout << "[OK] " << message << '\n';
        }
        else {
            ++stats.failed;
            std::cout << "[FAIL] " << message << '\n';
        }
    }

    std::string status_name(modeling::OperationStatus status) {
        switch (status) {
        case modeling::OperationStatus::Success:
            return "Success";
        case modeling::OperationStatus::Failed:
            return "Failed";
        case modeling::OperationStatus::NoChange:
            return "NoChange";
        case modeling::OperationStatus::Cancelled:
            return "Cancelled";
        }

        return "Unknown";
    }

    void print_mesh_counts(const geometry::LEM& mesh) {
        std::cout
            << "vertices: " << geometry::TopologyTraversal::vertices(mesh).size()
            << " | edges: " << geometry::TopologyTraversal::edges(mesh).size()
            << " | loops: " << geometry::TopologyTraversal::loops(mesh).size()
            << " | faces: " << geometry::TopologyTraversal::faces(mesh).size()
            << '\n';
    }

    void print_result(const modeling::OperationResult& result) {
        std::cout << "status: " << status_name(result.status()) << '\n';

        if (!result.message().empty()) {
            std::cout << "message: " << result.message() << '\n';
        }

        if (result.is_failure()) {
            std::cout << "error: " << result.error().message << '\n';
        }

        if (result.has_validation_report()) {
            const geometry::TopologyValidationReport& report = result.validation_report();

            std::cout
                << "validation issues: " << report.issues.size()
                << " | errors: " << report.error_count()
                << " | warnings: " << report.warning_count()
                << '\n';
        }
    }

    bool validate_mesh(const geometry::LEM& mesh) {
        const geometry::TopologyValidationReport report = geometry::TopologyValidator::validate(mesh);

        std::cout
            << "manual validation issues: " << report.issues.size()
            << " | errors: " << report.error_count()
            << " | warnings: " << report.warning_count()
            << '\n';

        return report.valid();
    }

    std::vector<geometry::VertexHandle> make_square_vertices(geometry::LEMEditor& editor, float offsetX = 0.0f) {
        std::vector<geometry::VertexHandle> vertices;

        vertices.push_back(editor.add_vertex(glm::vec3{ offsetX + 0.0f, 0.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ offsetX + 1.0f, 0.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ offsetX + 1.0f, 1.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ offsetX + 0.0f, 1.0f, 0.0f }));

        return vertices;
    }

    std::vector<geometry::EdgeHandle> make_square_edges(
        geometry::LEMEditor& editor,
        const std::vector<geometry::VertexHandle>& vertices) {
        std::vector<geometry::EdgeHandle> edges;

        edges.push_back(editor.find_or_create_edge(vertices[0], vertices[1]));
        edges.push_back(editor.find_or_create_edge(vertices[1], vertices[2]));
        edges.push_back(editor.find_or_create_edge(vertices[2], vertices[3]));
        edges.push_back(editor.find_or_create_edge(vertices[3], vertices[0]));

        return edges;
    }

    void test_fill_from_vertex_cycle(TestStats& stats) {
        std::cout << "\n=== FillHoleOp: vertex cycle ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> vertices = make_square_vertices(editor);
        editor.clear_diff();

        modeling::OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        modeling::FillHoleOp op(vertices);
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "operacao por vertices terminou com sucesso");
        check(stats, result.changed(), "operacao por vertices gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 4, "malha manteve 4 vertices");
        check(stats, geometry::TopologyTraversal::edges(mesh).size() == 4, "malha criou 4 edges");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 4, "malha criou 4 loops");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 1, "malha criou 1 face");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_fill_from_edge_cycle(TestStats& stats) {
        std::cout << "\n=== FillHoleOp: edge cycle ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> vertices = make_square_vertices(editor);
        std::vector<geometry::EdgeHandle> edges = make_square_edges(editor, vertices);
        editor.clear_diff();

        std::vector<geometry::EdgeHandle> shuffledEdges;
        shuffledEdges.push_back(edges[2]);
        shuffledEdges.push_back(edges[0]);
        shuffledEdges.push_back(edges[3]);
        shuffledEdges.push_back(edges[1]);

        modeling::OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        modeling::FillHoleOp op = modeling::FillHoleOp::edges(shuffledEdges);
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "operacao por edges terminou com sucesso");
        check(stats, result.changed(), "operacao por edges gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 4, "malha manteve 4 vertices");
        check(stats, geometry::TopologyTraversal::edges(mesh).size() == 4, "malha manteve 4 edges");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 4, "malha criou 4 loops");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 1, "malha criou 1 face");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_fill_from_selected_boundary_edges(TestStats& stats) {
        std::cout << "\n=== FillHoleOp: selected boundary edges ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> vertices = make_square_vertices(editor);
        std::vector<geometry::EdgeHandle> edges = make_square_edges(editor, vertices);

        for (geometry::EdgeHandle edge : edges) {
            const bool selected = editor.set_selected(edge, true);
            check(stats, selected, "edge selecionada para teste");
        }

        editor.clear_diff();

        modeling::OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        modeling::FillHoleOp op = modeling::FillHoleOp::selected_boundary_edges();
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "operacao por edges selecionadas terminou com sucesso");
        check(stats, result.changed(), "operacao por edges selecionadas gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 4, "malha manteve 4 vertices");
        check(stats, geometry::TopologyTraversal::edges(mesh).size() == 4, "malha manteve 4 edges");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 4, "malha criou 4 loops");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 1, "malha criou 1 face");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_invalid_duplicate_vertex_cycle(TestStats& stats) {
        std::cout << "\n=== FillHoleOp: invalid duplicate vertex cycle ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> vertices = make_square_vertices(editor);
        editor.clear_diff();

        std::vector<geometry::VertexHandle> invalidCycle;
        invalidCycle.push_back(vertices[0]);
        invalidCycle.push_back(vertices[1]);
        invalidCycle.push_back(vertices[1]);
        invalidCycle.push_back(vertices[3]);

        modeling::OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        modeling::FillHoleOp op(invalidCycle);
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_failure(), "ciclo com vertice duplicado falhou corretamente");
        check(stats, geometry::TopologyTraversal::faces(mesh).empty(), "nenhuma face foi criada no caso invalido");
        check(stats, geometry::TopologyTraversal::loops(mesh).empty(), "nenhum loop foi criado no caso invalido");
        check(stats, validate_mesh(mesh), "malha continuou valida apos falha");
    }

    void test_invalid_open_edge_chain(TestStats& stats) {
        std::cout << "\n=== FillHoleOp: invalid open edge chain ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> vertices;

        vertices.push_back(editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f }));

        std::vector<geometry::EdgeHandle> edges;

        edges.push_back(editor.find_or_create_edge(vertices[0], vertices[1]));
        edges.push_back(editor.find_or_create_edge(vertices[1], vertices[2]));
        edges.push_back(editor.find_or_create_edge(vertices[2], vertices[3]));

        editor.clear_diff();

        modeling::OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        modeling::FillHoleOp op = modeling::FillHoleOp::edges(edges);
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.status() == modeling::OperationStatus::NoChange, "cadeia aberta retornou NoChange");
        check(stats, geometry::TopologyTraversal::faces(mesh).empty(), "nenhuma face foi criada para cadeia aberta");
        check(stats, geometry::TopologyTraversal::loops(mesh).empty(), "nenhum loop foi criado para cadeia aberta");
        check(stats, validate_mesh(mesh), "malha continuou valida apos cadeia aberta");
    }

    void test_flip_winding(TestStats& stats) {
        std::cout << "\n=== FillHoleOp: flip winding ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> vertices = make_square_vertices(editor);
        editor.clear_diff();

        modeling::OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        modeling::FillHoleOp op(vertices);
        op.set_flip_winding(true);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "operacao com flip_winding terminou com sucesso");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 1, "flip_winding criou 1 face");
        check(stats, validate_mesh(mesh), "malha com flip_winding passou na validacao");

        const std::vector<geometry::FaceHandle> faces = geometry::TopologyTraversal::faces(mesh);

        if (!faces.empty()) {
            const glm::vec3 normal = mesh.face(faces.front()).normal;
            std::cout
                << "normal: "
                << normal.x << ", "
                << normal.y << ", "
                << normal.z << '\n';
        }
    }

}

int main() {
    std::cout << "=== Locus3D FillHoleOp Regression Test ===\n";

    TestStats stats;

    test_fill_from_vertex_cycle(stats);
    test_fill_from_edge_cycle(stats);
    test_fill_from_selected_boundary_edges(stats);
    test_invalid_duplicate_vertex_cycle(stats);
    test_invalid_open_edge_chain(stats);
    test_flip_winding(stats);

    std::cout << "\n=== Summary ===\n";
    std::cout << "passed: " << stats.passed << '\n';
    std::cout << "failed: " << stats.failed << '\n';

    if (stats.failed > 0) {
        return 1;
    }

    return 0;
}