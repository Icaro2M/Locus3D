#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"

#include <glm/geometric.hpp>
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
            std::cout << " | error";
        }

        if (result.has_validation_report()) {
            const TopologyValidationReport& report = result.validation_report();
            std::cout << " | validation issues: " << report.issues.size()
                << " | errors: " << report.error_count()
                << " | warnings: " << report.warning_count();
        }

        std::cout << '\n';
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

    std::size_t active_vertex_count(const LEM& mesh)
    {
        return TopologyTraversal::vertices(mesh).size();
    }

    std::size_t active_edge_count(const LEM& mesh)
    {
        return TopologyTraversal::edges(mesh).size();
    }

    std::size_t active_loop_count(const LEM& mesh)
    {
        return TopologyTraversal::loops(mesh).size();
    }

    std::size_t active_face_count(const LEM& mesh)
    {
        return TopologyTraversal::faces(mesh).size();
    }

    std::vector<VertexHandle> face_vertices(const LEM& mesh, FaceHandle face)
    {
        std::vector<VertexHandle> result;

        if (!mesh.is_valid(face)) {
            return result;
        }

        LoopHandle start = mesh.face(face).loop;
        if (!mesh.is_valid(start)) {
            return result;
        }

        LoopHandle current = start;

        do {
            if (!mesh.is_valid(current)) {
                result.clear();
                return result;
            }

            result.push_back(mesh.loop(current).vertex);
            current = mesh.loop(current).next;
        } while (mesh.is_valid(current) && current != start);

        return result;
    }

    glm::vec3 computed_face_normal(const LEM& mesh, FaceHandle face)
    {
        const std::vector<VertexHandle> vertices = face_vertices(mesh, face);

        if (vertices.size() < 3) {
            return { 0.0f, 0.0f, 0.0f };
        }

        const glm::vec3& p0 = mesh.vertex(vertices[0]).position;
        const glm::vec3& p1 = mesh.vertex(vertices[1]).position;
        const glm::vec3& p2 = mesh.vertex(vertices[2]).position;

        const glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
        const float length = glm::length(normal);

        if (length <= 0.000001f) {
            return { 0.0f, 0.0f, 0.0f };
        }

        return normal / length;
    }

    bool opposite_normals(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f)
    {
        return std::fabs(glm::dot(a, b) + 1.0f) <= epsilon;
    }

    bool same_normals(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f)
    {
        return std::fabs(glm::dot(a, b) - 1.0f) <= epsilon;
    }

    bool same_vertex_sequence(
        const std::vector<VertexHandle>& a,
        const std::vector<VertexHandle>& b)
    {
        if (a.size() != b.size()) {
            return false;
        }

        for (std::size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) {
                return false;
            }
        }

        return true;
    }

    bool reversed_vertex_sequence(
        const std::vector<VertexHandle>& before,
        const std::vector<VertexHandle>& after)
    {
        if (before.size() != after.size()) {
            return false;
        }

        if (before.empty()) {
            return true;
        }

        const std::size_t count = before.size();

        for (std::size_t offset = 0; offset < count; ++offset) {
            bool matches = true;

            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t reversedIndex = (count + offset - i) % count;

                if (after[i] != before[reversedIndex]) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                return true;
            }
        }

        return false;
    }

    void test_triangle_flip(TestStats& stats)
    {
        print_header("FlipFaceOp: triangulo");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2 });
        editor.rebuild_face_normals();
        editor.clear_diff();

        expect(stats, mesh.is_valid(face), "triangulo criado");
        expect(stats, active_vertex_count(mesh) == 3, "triangulo tem 3 vertices");
        expect(stats, active_edge_count(mesh) == 3, "triangulo tem 3 edges");
        expect(stats, active_loop_count(mesh) == 3, "triangulo tem 3 loops");
        expect(stats, active_face_count(mesh) == 1, "triangulo tem 1 face");
        expect(stats, validate_mesh(mesh, "validacao antes do flip"), "malha inicial valida");

        const std::vector<VertexHandle> beforeVertices = face_vertices(mesh, face);
        const glm::vec3 beforeNormal = computed_face_normal(mesh, face);

        OperationContext context = make_context(mesh);
        FlipFaceOp op(face);
        OperationResult result = op.execute(context);

        print_result(result);

        const std::vector<VertexHandle> afterVertices = face_vertices(mesh, face);
        const glm::vec3 afterNormal = computed_face_normal(mesh, face);

        expect(stats, result.is_success(), "flip de triangulo retorna sucesso");
        expect(stats, result.changed(), "flip de triangulo registra diff");
        expect(stats, mesh.is_valid(face), "face continua valida");
        expect(stats, active_vertex_count(mesh) == 3, "flip nao altera quantidade de vertices");
        expect(stats, active_edge_count(mesh) == 3, "flip nao altera quantidade de edges");
        expect(stats, active_loop_count(mesh) == 3, "flip nao altera quantidade de loops");
        expect(stats, active_face_count(mesh) == 1, "flip nao altera quantidade de faces");
        expect(stats, reversed_vertex_sequence(beforeVertices, afterVertices), "ordem dos vertices foi invertida");
        expect(stats, opposite_normals(beforeNormal, afterNormal), "normal calculada foi invertida");
        expect(stats, validate_mesh(mesh, "validacao depois do flip"), "malha valida depois do flip");
    }

    void test_quad_flip(TestStats& stats)
    {
        print_header("FlipFaceOp: quad");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });
        editor.rebuild_face_normals();
        editor.clear_diff();

        expect(stats, mesh.is_valid(face), "quad criado");
        expect(stats, active_vertex_count(mesh) == 4, "quad tem 4 vertices");
        expect(stats, active_edge_count(mesh) == 4, "quad tem 4 edges");
        expect(stats, active_loop_count(mesh) == 4, "quad tem 4 loops");
        expect(stats, active_face_count(mesh) == 1, "quad tem 1 face");
        expect(stats, validate_mesh(mesh, "validacao antes do flip"), "malha inicial valida");

        const std::vector<VertexHandle> beforeVertices = face_vertices(mesh, face);
        const glm::vec3 beforeNormal = computed_face_normal(mesh, face);

        OperationContext context = make_context(mesh);
        FlipFaceOp op(face);
        OperationResult result = op.execute(context);

        print_result(result);

        const std::vector<VertexHandle> afterVertices = face_vertices(mesh, face);
        const glm::vec3 afterNormal = computed_face_normal(mesh, face);

        expect(stats, result.is_success(), "flip de quad retorna sucesso");
        expect(stats, result.changed(), "flip de quad registra diff");
        expect(stats, mesh.is_valid(face), "face continua valida");
        expect(stats, active_vertex_count(mesh) == 4, "flip nao altera quantidade de vertices");
        expect(stats, active_edge_count(mesh) == 4, "flip nao altera quantidade de edges");
        expect(stats, active_loop_count(mesh) == 4, "flip nao altera quantidade de loops");
        expect(stats, active_face_count(mesh) == 1, "flip nao altera quantidade de faces");
        expect(stats, reversed_vertex_sequence(beforeVertices, afterVertices), "ordem dos vertices foi invertida");
        expect(stats, opposite_normals(beforeNormal, afterNormal), "normal calculada foi invertida");
        expect(stats, validate_mesh(mesh, "validacao depois do flip"), "malha valida depois do flip");
    }

    void test_double_flip(TestStats& stats)
    {
        print_header("FlipFaceOp: duplo flip");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2 });
        editor.rebuild_face_normals();
        editor.clear_diff();

        expect(stats, mesh.is_valid(face), "triangulo criado");
        expect(stats, validate_mesh(mesh, "validacao antes do duplo flip"), "malha inicial valida");

        const std::vector<VertexHandle> originalVertices = face_vertices(mesh, face);
        const glm::vec3 originalNormal = computed_face_normal(mesh, face);

        OperationContext context = make_context(mesh);

        FlipFaceOp firstFlip(face);
        OperationResult firstResult = firstFlip.execute(context);

        print_result(firstResult);

        FlipFaceOp secondFlip(face);
        OperationResult secondResult = secondFlip.execute(context);

        print_result(secondResult);

        const std::vector<VertexHandle> finalVertices = face_vertices(mesh, face);
        const glm::vec3 finalNormal = computed_face_normal(mesh, face);

        expect(stats, firstResult.is_success(), "primeiro flip retorna sucesso");
        expect(stats, secondResult.is_success(), "segundo flip retorna sucesso");
        expect(stats, same_vertex_sequence(originalVertices, finalVertices), "duplo flip restaura ordem original");
        expect(stats, same_normals(originalNormal, finalNormal), "duplo flip restaura normal original");
        expect(stats, active_vertex_count(mesh) == 3, "duplo flip nao altera vertices");
        expect(stats, active_edge_count(mesh) == 3, "duplo flip nao altera edges");
        expect(stats, active_loop_count(mesh) == 3, "duplo flip nao altera loops");
        expect(stats, active_face_count(mesh) == 1, "duplo flip nao altera faces");
        expect(stats, validate_mesh(mesh, "validacao depois do duplo flip"), "malha valida depois do duplo flip");
    }

    void test_invalid_face_no_change(TestStats& stats)
    {
        print_header("FlipFaceOp: face invalida");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle validFace = editor.add_face({ v0, v1, v2 });
        FaceHandle invalidFace{};

        editor.clear_diff();

        expect(stats, mesh.is_valid(validFace), "face valida criada");
        expect(stats, !mesh.is_valid(invalidFace), "handle default nao aponta para face valida");
        expect(stats, validate_mesh(mesh, "validacao antes do no change"), "malha inicial valida");

        OperationContext context = make_context(mesh);
        FlipFaceOp op(invalidFace);
        OperationResult result = op.execute(context);

        print_result(result);

        expect(stats, result.status() == OperationStatus::NoChange, "face invalida retorna NoChange");
        expect(stats, !result.changed(), "face invalida nao altera diff");
        expect(stats, mesh.is_valid(validFace), "face valida continua existindo");
        expect(stats, active_vertex_count(mesh) == 3, "NoChange nao altera vertices");
        expect(stats, active_edge_count(mesh) == 3, "NoChange nao altera edges");
        expect(stats, active_loop_count(mesh) == 3, "NoChange nao altera loops");
        expect(stats, active_face_count(mesh) == 1, "NoChange nao altera faces");
        expect(stats, validate_mesh(mesh, "validacao depois do no change"), "malha continua valida");
    }

    void test_shared_edge_faces(TestStats& stats)
    {
        print_header("FlipFaceOp: faces compartilhando edge");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle f0 = editor.add_face({ v0, v1, v2 });
        FaceHandle f1 = editor.add_face({ v0, v2, v3 });

        editor.rebuild_face_normals();
        editor.clear_diff();

        expect(stats, mesh.is_valid(f0), "primeira face criada");
        expect(stats, mesh.is_valid(f1), "segunda face criada");
        expect(stats, active_vertex_count(mesh) == 4, "malha compartilhada tem 4 vertices");
        expect(stats, active_edge_count(mesh) == 5, "malha compartilhada tem 5 edges");
        expect(stats, active_loop_count(mesh) == 6, "malha compartilhada tem 6 loops");
        expect(stats, active_face_count(mesh) == 2, "malha compartilhada tem 2 faces");
        expect(stats, validate_mesh(mesh, "validacao antes do flip"), "malha inicial valida");

        const glm::vec3 f0BeforeNormal = computed_face_normal(mesh, f0);
        const glm::vec3 f1BeforeNormal = computed_face_normal(mesh, f1);

        OperationContext context = make_context(mesh);
        FlipFaceOp op(f0);
        OperationResult result = op.execute(context);

        print_result(result);

        const glm::vec3 f0AfterNormal = computed_face_normal(mesh, f0);
        const glm::vec3 f1AfterNormal = computed_face_normal(mesh, f1);

        expect(stats, result.is_success(), "flip em uma face compartilhada retorna sucesso");
        expect(stats, result.changed(), "flip em face compartilhada registra diff");
        expect(stats, mesh.is_valid(f0), "face flipada continua valida");
        expect(stats, mesh.is_valid(f1), "face vizinha continua valida");
        expect(stats, opposite_normals(f0BeforeNormal, f0AfterNormal), "normal da face flipada inverte");
        expect(stats, same_normals(f1BeforeNormal, f1AfterNormal), "normal da face vizinha permanece igual");
        expect(stats, active_vertex_count(mesh) == 4, "flip compartilhado nao altera vertices");
        expect(stats, active_edge_count(mesh) == 5, "flip compartilhado nao altera edges");
        expect(stats, active_loop_count(mesh) == 6, "flip compartilhado nao altera loops");
        expect(stats, active_face_count(mesh) == 2, "flip compartilhado nao altera faces");
        expect(stats, validate_mesh(mesh, "validacao depois do flip"), "malha valida depois do flip compartilhado");
    }

}

int main()
{
    std::cout << "=== Locus3D FlipFaceOp Regression Test ===\n";

    TestStats stats;

    test_triangle_flip(stats);
    test_quad_flip(stats);
    test_double_flip(stats);
    test_invalid_face_no_change(stats);
    test_shared_edge_faces(stats);

    std::cout << "\n=== Resultado final ===\n";
    std::cout << "Passou: " << stats.passed << '\n';
    std::cout << "Falhou: " << stats.failed << '\n';

    if (stats.failed == 0) {
        std::cout << "\nTodos os testes de FlipFaceOp passaram.\n";
        return 0;
    }

    std::cout << "\nAlguns testes de FlipFaceOp falharam.\n";
    return 1;
}