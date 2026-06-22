#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/topology/MergeVerticesOp.h"

#include <glm/vec3.hpp>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

    using namespace locus::kernel;
    using namespace locus::kernel::geometry;
    using namespace locus::kernel::modeling;

    struct TestStats {
        int passed = 0;
        int failed = 0;
    };

    void print_header(std::string_view title)
    {
        std::cout << "\n=== " << title << " ===\n";
    }

    void expect(TestStats& stats, bool condition, std::string_view message)
    {
        if (condition) {
            ++stats.passed;
            std::cout << "[OK] " << message << '\n';
        }
        else {
            ++stats.failed;
            std::cout << "[FAIL] " << message << '\n';
        }
    }

    const char* status_name(OperationStatus status)
    {
        switch (status) {
        case OperationStatus::Success:
            return "Success";
        case OperationStatus::Failed:
            return "Failed";
        case OperationStatus::NoChange:
            return "NoChange";
        case OperationStatus::Cancelled:
            return "Cancelled";
        }

        return "Unknown";
    }

    void print_result(const OperationResult& result)
    {
        std::cout << "status: " << status_name(result.status())
            << " | changed: " << (result.changed() ? "true" : "false")
            << " | diff: " << result.diff().size();

        if (!result.message().empty()) {
            std::cout << " | message: " << result.message();
        }

        if (result.is_failure()) {
            std::cout << " | error: " << result.error().message;
        }

        if (result.has_validation_report()) {
            const TopologyValidationReport& report = result.validation_report();
            std::cout << " | validation issues: " << report.issues.size()
                << " | errors: " << report.error_count()
                << " | warnings: " << report.warning_count();
        }

        std::cout << '\n';
    }

    bool near_vec3(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f)
    {
        return std::fabs(a.x - b.x) <= epsilon
            && std::fabs(a.y - b.y) <= epsilon
            && std::fabs(a.z - b.z) <= epsilon;
    }

    std::size_t active_vertex_count(const LEM& mesh)
    {
        return TopologyTraversal::vertices(mesh).size();
    }

    std::size_t active_edge_count(const LEM& mesh)
    {
        return TopologyTraversal::edges(mesh).size();
    }

    std::size_t active_face_count(const LEM& mesh)
    {
        return TopologyTraversal::faces(mesh).size();
    }

    bool validate_mesh(const LEM& mesh, std::string_view label)
    {
        const TopologyValidationReport report = TopologyValidator::validate(mesh);

        std::cout << label
            << " | issues: " << report.issues.size()
            << " | errors: " << report.error_count()
            << " | warnings: " << report.warning_count()
            << '\n';

        return report.valid();
    }

    OperationContext make_context(LEM& mesh)
    {
        OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;
        return context;
    }

    void test_pair_merge(TestStats& stats)
    {
        print_header("MergeVerticesOp: Pair");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2 });
        editor.clear_diff();

        expect(stats, mesh.is_valid(face), "triangulo inicial criado");
        expect(stats, active_vertex_count(mesh) == 3, "malha inicia com 3 vertices ativos");
        expect(stats, validate_mesh(mesh, "validacao antes do merge"), "malha inicial valida");

        OperationContext context = make_context(mesh);
        MergeVerticesOp op(v1, v0);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.is_success(), "Pair retorna sucesso");
        expect(stats, result.changed(), "Pair registra diff nao vazio");
        expect(stats, mesh.is_valid(v0), "target continua valido");
        expect(stats, !mesh.is_valid(v1), "source deixa de ser valido");
        expect(stats, active_vertex_count(mesh) == 2, "Pair reduz vertices ativos para 2");
        expect(stats, validate_mesh(mesh, "validacao depois do Pair"), "malha valida depois do Pair");
    }

    void test_pair_at_position(TestStats& stats)
    {
        print_header("MergeVerticesOp: PairAtPosition");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2 });
        editor.clear_diff();

        const glm::vec3 mergedPosition{ 0.5f, 0.5f, 2.0f };

        expect(stats, mesh.is_valid(face), "triangulo inicial criado");
        expect(stats, validate_mesh(mesh, "validacao antes do merge"), "malha inicial valida");

        OperationContext context = make_context(mesh);
        MergeVerticesOp op(v1, v0, mergedPosition);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.is_success(), "PairAtPosition retorna sucesso");
        expect(stats, result.changed(), "PairAtPosition registra diff nao vazio");
        expect(stats, mesh.is_valid(v0), "target continua valido");
        expect(stats, !mesh.is_valid(v1), "source deixa de ser valido");
        expect(stats, near_vec3(mesh.vertex(v0).position, mergedPosition), "target recebe posicao final customizada");
        expect(stats, validate_mesh(mesh, "validacao depois do PairAtPosition"), "malha valida depois do PairAtPosition");
    }

    void test_distance_merge(TestStats& stats)
    {
        print_header("MergeVerticesOp: Distance");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 0.02f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 2.0f, 0.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 3.0f, 0.0f, 0.0f });

        editor.clear_diff();

        expect(stats, active_vertex_count(mesh) == 4, "malha inicia com 4 vertices ativos");
        expect(stats, validate_mesh(mesh, "validacao antes do merge"), "malha inicial valida");

        OperationContext context = make_context(mesh);
        MergeVerticesOp op(0.05f);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.is_success(), "Distance retorna sucesso");
        expect(stats, result.changed(), "Distance registra diff nao vazio");
        expect(stats, active_vertex_count(mesh) == 3, "Distance funde apenas o par proximo");
        expect(stats, mesh.is_valid(v2), "vertice distante v2 continua valido");
        expect(stats, mesh.is_valid(v3), "vertice distante v3 continua valido");
        expect(stats, validate_mesh(mesh, "validacao depois do Distance"), "malha valida depois do Distance");
    }

    void test_vertex_set_distance_merge(TestStats& stats)
    {
        print_header("MergeVerticesOp: VertexSetDistance");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 0.02f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 2.0f, 0.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 2.02f, 0.0f, 0.0f });

        editor.clear_diff();

        expect(stats, active_vertex_count(mesh) == 4, "malha inicia com 4 vertices ativos");
        expect(stats, validate_mesh(mesh, "validacao antes do merge"), "malha inicial valida");

        OperationContext context = make_context(mesh);
        MergeVerticesOp op(std::vector<VertexHandle>{ v0, v1 }, 0.05f);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.is_success(), "VertexSetDistance retorna sucesso");
        expect(stats, result.changed(), "VertexSetDistance registra diff nao vazio");
        expect(stats, active_vertex_count(mesh) == 3, "VertexSetDistance funde apenas vertices do conjunto passado");
        expect(stats, mesh.is_valid(v2), "v2 fora do conjunto continua valido");
        expect(stats, mesh.is_valid(v3), "v3 fora do conjunto continua valido");
        expect(stats, validate_mesh(mesh, "validacao depois do VertexSetDistance"), "malha valida depois do VertexSetDistance");
    }

    void test_no_change_invalid_pair(TestStats& stats)
    {
        print_header("MergeVerticesOp: NoChange com par igual");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        editor.clear_diff();

        OperationContext context = make_context(mesh);
        MergeVerticesOp op(v0, v0);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.status() == OperationStatus::NoChange, "source e target iguais retornam NoChange");
        expect(stats, !result.changed(), "NoChange nao registra mudanca");
        expect(stats, active_vertex_count(mesh) == 1, "malha continua com 1 vertice");
        expect(stats, validate_mesh(mesh, "validacao depois do NoChange"), "malha continua valida");
    }

    void test_no_change_distance(TestStats& stats)
    {
        print_header("MergeVerticesOp: NoChange com distancia sem candidatos");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 5.0f, 0.0f, 0.0f });

        editor.clear_diff();

        OperationContext context = make_context(mesh);
        MergeVerticesOp op(0.01f);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.status() == OperationStatus::NoChange, "distancia sem candidatos retorna NoChange");
        expect(stats, !result.changed(), "NoChange nao registra mudanca");
        expect(stats, mesh.is_valid(v0), "v0 continua valido");
        expect(stats, mesh.is_valid(v1), "v1 continua valido");
        expect(stats, active_vertex_count(mesh) == 2, "malha continua com 2 vertices");
        expect(stats, validate_mesh(mesh, "validacao depois do NoChange"), "malha continua valida");
    }

    void test_distance_with_connected_geometry(TestStats& stats)
    {
        print_header("MergeVerticesOp: Distance em geometria com faces");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });
        VertexHandle v4 = editor.add_vertex({ 0.01f, 1.0f, 0.0f });

        FaceHandle f0 = editor.add_face({ v0, v1, v2, v3 });
        FaceHandle f1 = editor.add_face({ v0, v1, v2, v4 });

        editor.clear_diff();

        expect(stats, mesh.is_valid(f0), "primeiro quad criado");
        expect(stats, mesh.is_valid(f1), "segundo quad criado");
        expect(stats, active_vertex_count(mesh) == 5, "malha inicia com 5 vertices ativos");
        expect(stats, active_face_count(mesh) == 2, "malha inicia com 2 faces ativas");
        expect(stats, validate_mesh(mesh, "validacao antes do merge"), "malha inicial valida");

        OperationContext context = make_context(mesh);
        MergeVerticesOp op(0.05f);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.is_success(), "Distance em malha com faces retorna sucesso");
        expect(stats, result.changed(), "Distance em malha com faces registra diff nao vazio");
        expect(stats, active_vertex_count(mesh) == 4, "vertices proximos em faces foram fundidos");
        expect(stats, active_face_count(mesh) >= 1, "malha ainda possui pelo menos uma face ativa");
        expect(stats, validate_mesh(mesh, "validacao depois do Distance com faces"), "malha valida depois do Distance com faces");
    }

}

int main()
{
    std::cout << "=== Locus3D MergeVerticesOp Regression Test ===\n";

    TestStats stats;

    test_pair_merge(stats);
    test_pair_at_position(stats);
    test_distance_merge(stats);
    test_vertex_set_distance_merge(stats);
    test_no_change_invalid_pair(stats);
    test_no_change_distance(stats);
    test_distance_with_connected_geometry(stats);

    std::cout << "\n=== Resultado final ===\n";
    std::cout << "Passou: " << stats.passed << '\n';
    std::cout << "Falhou: " << stats.failed << '\n';

    if (stats.failed == 0) {
        std::cout << "\nTodos os testes de MergeVerticesOp passaram.\n";
        return 0;
    }

    std::cout << "\nAlguns testes de MergeVerticesOp falharam.\n";
    return 1;
}