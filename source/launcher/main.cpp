#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/topology/BridgeEdgeOp.h"

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

    modeling::OperationContext make_context(geometry::LEM& mesh) {
        modeling::OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;
        return context;
    }

    std::vector<geometry::VertexHandle> make_square_cycle(
        geometry::LEMEditor& editor,
        float z,
        float size = 1.0f) {
        std::vector<geometry::VertexHandle> vertices;

        vertices.push_back(editor.add_vertex(glm::vec3{ -size, -size, z }));
        vertices.push_back(editor.add_vertex(glm::vec3{ size, -size, z }));
        vertices.push_back(editor.add_vertex(glm::vec3{ size,  size, z }));
        vertices.push_back(editor.add_vertex(glm::vec3{ -size,  size, z }));

        return vertices;
    }

    std::vector<geometry::VertexHandle> make_triangle_cycle(
        geometry::LEMEditor& editor,
        float z,
        float size = 1.0f) {
        std::vector<geometry::VertexHandle> vertices;

        vertices.push_back(editor.add_vertex(glm::vec3{ 0.0f,  size, z }));
        vertices.push_back(editor.add_vertex(glm::vec3{ size, -size, z }));
        vertices.push_back(editor.add_vertex(glm::vec3{ -size, -size, z }));

        return vertices;
    }

    std::vector<geometry::EdgeHandle> make_cycle_edges(
        geometry::LEMEditor& editor,
        const std::vector<geometry::VertexHandle>& vertices) {
        std::vector<geometry::EdgeHandle> edges;

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const std::size_t next = (i + 1) % vertices.size();
            edges.push_back(editor.find_or_create_edge(vertices[i], vertices[next]));
        }

        return edges;
    }

    std::vector<geometry::EdgeHandle> make_open_chain_edges(
        geometry::LEMEditor& editor,
        const std::vector<geometry::VertexHandle>& vertices) {
        std::vector<geometry::EdgeHandle> edges;

        for (std::size_t i = 0; i + 1 < vertices.size(); ++i) {
            edges.push_back(editor.find_or_create_edge(vertices[i], vertices[i + 1]));
        }

        return edges;
    }

    void select_edges(
        TestStats& stats,
        geometry::LEMEditor& editor,
        const std::vector<geometry::EdgeHandle>& edges) {
        for (geometry::EdgeHandle edge : edges) {
            const bool selected = editor.set_selected(edge, true);
            check(stats, selected, "edge selecionada para teste");
        }
    }

    void print_face_normals(const geometry::LEM& mesh) {
        const std::vector<geometry::FaceHandle> faces = geometry::TopologyTraversal::faces(mesh);

        for (std::size_t i = 0; i < faces.size(); ++i) {
            const glm::vec3 normal = mesh.face(faces[i]).normal;

            std::cout
                << "face " << i
                << " normal: "
                << normal.x << ", "
                << normal.y << ", "
                << normal.z << '\n';
        }
    }

    void test_bridge_from_vertex_cycles(TestStats& stats) {
        std::cout << "\n=== BridgeEdgeOp: vertex cycles ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> bottom = make_square_cycle(editor, 0.0f);
        std::vector<geometry::VertexHandle> top = make_square_cycle(editor, 1.0f);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::BridgeEdgeOp op(bottom, top);
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);
        print_face_normals(mesh);

        check(stats, result.is_success(), "bridge por vertices terminou com sucesso");
        check(stats, result.changed(), "bridge por vertices gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "malha manteve 8 vertices");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 4, "bridge fechado criou 4 faces laterais");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 16, "bridge fechado criou 16 loops");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_bridge_from_edge_cycles(TestStats& stats) {
        std::cout << "\n=== BridgeEdgeOp: edge cycles ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> bottom = make_square_cycle(editor, 0.0f);
        std::vector<geometry::VertexHandle> top = make_square_cycle(editor, 1.0f);

        std::vector<geometry::EdgeHandle> bottomEdges = make_cycle_edges(editor, bottom);
        std::vector<geometry::EdgeHandle> topEdges = make_cycle_edges(editor, top);

        std::vector<geometry::EdgeHandle> shuffledBottomEdges;
        shuffledBottomEdges.push_back(bottomEdges[2]);
        shuffledBottomEdges.push_back(bottomEdges[0]);
        shuffledBottomEdges.push_back(bottomEdges[3]);
        shuffledBottomEdges.push_back(bottomEdges[1]);

        std::vector<geometry::EdgeHandle> shuffledTopEdges;
        shuffledTopEdges.push_back(topEdges[1]);
        shuffledTopEdges.push_back(topEdges[3]);
        shuffledTopEdges.push_back(topEdges[0]);
        shuffledTopEdges.push_back(topEdges[2]);

        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::BridgeEdgeOp op = modeling::BridgeEdgeOp::edges(
            shuffledBottomEdges,
            shuffledTopEdges);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "bridge por edges terminou com sucesso");
        check(stats, result.changed(), "bridge por edges gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "malha manteve 8 vertices");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 4, "bridge por edges criou 4 faces laterais");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 16, "bridge por edges criou 16 loops");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_bridge_from_selected_boundary_edges(TestStats& stats) {
        std::cout << "\n=== BridgeEdgeOp: selected boundary edges ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> bottom = make_square_cycle(editor, 0.0f);
        std::vector<geometry::VertexHandle> top = make_square_cycle(editor, 1.0f);

        std::vector<geometry::EdgeHandle> bottomEdges = make_cycle_edges(editor, bottom);
        std::vector<geometry::EdgeHandle> topEdges = make_cycle_edges(editor, top);

        select_edges(stats, editor, bottomEdges);
        select_edges(stats, editor, topEdges);

        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::BridgeEdgeOp op = modeling::BridgeEdgeOp::selected_boundary_edges();
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "bridge por edges selecionadas terminou com sucesso");
        check(stats, result.changed(), "bridge por edges selecionadas gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "malha manteve 8 vertices");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 4, "bridge por selecao criou 4 faces laterais");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 16, "bridge por selecao criou 16 loops");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_open_bridge_from_edges(TestStats& stats) {
        std::cout << "\n=== BridgeEdgeOp: open edge chains ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> firstChain;
        firstChain.push_back(editor.add_vertex(glm::vec3{ -1.0f, 0.0f, 0.0f }));
        firstChain.push_back(editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f }));
        firstChain.push_back(editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f }));

        std::vector<geometry::VertexHandle> secondChain;
        secondChain.push_back(editor.add_vertex(glm::vec3{ -1.0f, 0.0f, 1.0f }));
        secondChain.push_back(editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 1.0f }));
        secondChain.push_back(editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 1.0f }));

        std::vector<geometry::EdgeHandle> firstEdges = make_open_chain_edges(editor, firstChain);
        std::vector<geometry::EdgeHandle> secondEdges = make_open_chain_edges(editor, secondChain);

        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::BridgeEdgeOp op = modeling::BridgeEdgeOp::edges(firstEdges, secondEdges);
        op.set_closed(false);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "bridge aberto terminou com sucesso");
        check(stats, result.changed(), "bridge aberto gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 6, "malha aberta manteve 6 vertices");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 2, "bridge aberto criou 2 faces");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 8, "bridge aberto criou 8 loops");
        check(stats, validate_mesh(mesh), "malha aberta final passou na validacao");
    }

    void test_flip_second_cycle(TestStats& stats) {
        std::cout << "\n=== BridgeEdgeOp: flip second cycle ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> bottom = make_square_cycle(editor, 0.0f);
        std::vector<geometry::VertexHandle> top = make_square_cycle(editor, 1.0f);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::BridgeEdgeOp op(bottom, top);
        op.set_flip_second_cycle(true);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);
        print_face_normals(mesh);

        check(stats, result.is_success(), "bridge com flip_second_cycle terminou com sucesso");
        check(stats, result.changed(), "bridge com flip_second_cycle gerou diff");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 4, "bridge com flip criou 4 faces");
        check(stats, validate_mesh(mesh), "malha com flip passou na validacao");
    }

    void test_twist_offset(TestStats& stats) {
        std::cout << "\n=== BridgeEdgeOp: twist offset ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> bottom = make_square_cycle(editor, 0.0f);
        std::vector<geometry::VertexHandle> top = make_square_cycle(editor, 1.0f);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::BridgeEdgeOp op(bottom, top);
        op.set_twist_offset(1);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "bridge com twist_offset terminou com sucesso");
        check(stats, result.changed(), "bridge com twist_offset gerou diff");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 4, "bridge com twist criou 4 faces");
        check(stats, validate_mesh(mesh), "malha com twist passou na validacao");
    }

    void test_invalid_incompatible_cycles(TestStats& stats) {
        std::cout << "\n=== BridgeEdgeOp: invalid incompatible cycles ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        std::vector<geometry::VertexHandle> square = make_square_cycle(editor, 0.0f);
        std::vector<geometry::VertexHandle> triangle = make_triangle_cycle(editor, 1.0f);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::BridgeEdgeOp op(square, triangle);
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_failure(), "ciclos incompatíveis falharam corretamente");
        check(stats, geometry::TopologyTraversal::faces(mesh).empty(), "nenhuma face foi criada no caso invalido");
        check(stats, geometry::TopologyTraversal::loops(mesh).empty(), "nenhum loop foi criado no caso invalido");
        check(stats, validate_mesh(mesh), "malha continuou valida apos falha");
    }

}

int main() {
    std::cout << "=== Locus3D BridgeEdgeOp Regression Test ===\n";

    TestStats stats;

    test_bridge_from_vertex_cycles(stats);
    test_bridge_from_edge_cycles(stats);
    test_bridge_from_selected_boundary_edges(stats);
    test_open_bridge_from_edges(stats);
    test_flip_second_cycle(stats);
    test_twist_offset(stats);
    test_invalid_incompatible_cycles(stats);

    std::cout << "\n=== Summary ===\n";
    std::cout << "passed: " << stats.passed << '\n';
    std::cout << "failed: " << stats.failed << '\n';

    if (stats.failed > 0) {
        return 1;
    }

    return 0;
}