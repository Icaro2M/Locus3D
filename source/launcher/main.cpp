#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/topology/LoopCutOp.h"

#include <glm/glm.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

namespace geometry = locus::kernel::geometry;
namespace modeling = locus::kernel::modeling;

static std::string_view status_name(modeling::OperationStatus status)
{
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

static void print_vec3(const glm::vec3& value)
{
    std::cout
        << std::fixed << std::setprecision(3)
        << value.x << ", " << value.y << ", " << value.z;
}

static void print_counts(const geometry::LEM& mesh)
{
    const auto vertices = geometry::TopologyTraversal::vertices(mesh);
    const auto edges = geometry::TopologyTraversal::edges(mesh);
    const auto loops = geometry::TopologyTraversal::loops(mesh);
    const auto faces = geometry::TopologyTraversal::faces(mesh);

    std::cout << "vertices: " << vertices.size()
        << " | edges: " << edges.size()
        << " | loops: " << loops.size()
        << " | faces: " << faces.size()
        << '\n';
}

static void print_vertices(const geometry::LEM& mesh)
{
    const auto vertices = geometry::TopologyTraversal::vertices(mesh);

    std::cout << "\n=== Vertices ===\n";

    for (geometry::VertexHandle vertexHandle : vertices) {
        const geometry::Vertex& vertex = mesh.vertex(vertexHandle);

        std::cout << "v" << vertexHandle.id.value << ": ";
        print_vec3(vertex.position);
        std::cout << '\n';
    }
}

static void print_faces(const geometry::LEM& mesh)
{
    const auto faces = geometry::TopologyTraversal::faces(mesh);

    std::cout << "\n=== Faces ===\n";

    for (geometry::FaceHandle faceHandle : faces) {
        const geometry::Face& face = mesh.face(faceHandle);
        const auto vertices = geometry::TopologyTraversal::face_vertices(mesh, faceHandle);

        std::cout << "f" << faceHandle.id.value << " vertices:";

        for (geometry::VertexHandle vertexHandle : vertices) {
            std::cout << " v" << vertexHandle.id.value;
        }

        std::cout << " | normal: ";
        print_vec3(face.normal);
        std::cout << '\n';
    }
}

static void print_validation_report(const geometry::TopologyValidationReport& report)
{
    std::cout << "validation issues: " << report.issues.size()
        << " | errors: " << report.error_count()
        << " | warnings: " << report.warning_count()
        << '\n';

    for (const geometry::TopologyIssue& issue : report.issues) {
        std::cout << "- " << issue.message << '\n';
    }
}

static bool expect(bool condition, std::string_view message)
{
    if (condition) {
        std::cout << "[OK] " << message << '\n';
        return true;
    }

    std::cout << "[FAIL] " << message << '\n';
    return false;
}

int main()
{
    std::cout << "=== Locus3D LoopCutOp Regression Test ===\n\n";

    bool ok = true;

    geometry::LEM mesh;
    geometry::LEMEditor editor(mesh);

    geometry::VertexHandle v0 = editor.add_vertex(glm::vec3(-1.0f, 0.0f, -1.0f));
    geometry::VertexHandle v1 = editor.add_vertex(glm::vec3(1.0f, 0.0f, -1.0f));
    geometry::VertexHandle v2 = editor.add_vertex(glm::vec3(1.0f, 0.0f, 1.0f));
    geometry::VertexHandle v3 = editor.add_vertex(glm::vec3(-1.0f, 0.0f, 1.0f));

    geometry::FaceHandle face = editor.add_face({ v0, v1, v2, v3 });
    editor.rebuild_face_normals();
    editor.clear_diff();

    geometry::EdgeHandle bottomEdge = mesh.find_edge(v0, v1);
    geometry::EdgeHandle topEdge = mesh.find_edge(v2, v3);

    ok &= expect(mesh.is_valid(v0), "v0 valido");
    ok &= expect(mesh.is_valid(v1), "v1 valido");
    ok &= expect(mesh.is_valid(v2), "v2 valido");
    ok &= expect(mesh.is_valid(v3), "v3 valido");
    ok &= expect(mesh.is_valid(face), "face quad criada");
    ok &= expect(mesh.is_valid(bottomEdge), "edge inferior encontrada");
    ok &= expect(mesh.is_valid(topEdge), "edge superior encontrada");

    std::cout << "\n=== Antes do LoopCutOp ===\n";
    print_counts(mesh);
    print_vertices(mesh);
    print_faces(mesh);

    geometry::TopologyValidationReport beforeReport =
        geometry::TopologyValidator::validate(mesh);

    std::cout << "\n=== Validacao antes ===\n";
    print_validation_report(beforeReport);
    ok &= expect(beforeReport.valid(), "malha inicial valida");

    modeling::LoopCutOp op({ bottomEdge, topEdge });
    op.set_cuts(1);
    op.set_factor(0.5f);
    op.set_even_spacing(true);

    modeling::OperationContext context;
    context.mesh = &mesh;
    context.validateAfterExecute = true;
    context.rebuildNormals = true;
    context.allowNonManifold = true;

    modeling::OperationResult result = op.execute(context);

    std::cout << "\n=== Resultado do LoopCutOp por edges explicitas ===\n";
    std::cout << "status: " << status_name(result.status()) << '\n';

    if (!result.message().empty()) {
        std::cout << "message: " << result.message() << '\n';
    }

    ok &= expect(result.is_success(), "LoopCutOp terminou com sucesso");
    ok &= expect(result.changed(), "LoopCutOp gerou diff");

    std::cout << "\n=== Depois do LoopCutOp ===\n";
    print_counts(mesh);
    print_vertices(mesh);
    print_faces(mesh);

    const auto verticesAfter = geometry::TopologyTraversal::vertices(mesh);
    const auto facesAfter = geometry::TopologyTraversal::faces(mesh);
    const auto loopsAfter = geometry::TopologyTraversal::loops(mesh);

    ok &= expect(verticesAfter.size() == 6, "loop cut criou 2 vertices novos");
    ok &= expect(facesAfter.size() == 2, "loop cut dividiu o quad em 2 faces");
    ok &= expect(loopsAfter.size() == 8, "duas faces quad possuem 8 loops no total");

    geometry::TopologyValidationReport afterReport =
        geometry::TopologyValidator::validate(mesh);

    std::cout << "\n=== Validacao depois ===\n";
    print_validation_report(afterReport);
    ok &= expect(afterReport.valid(), "malha final valida");

    std::cout << "\n=== Teste de selected_edges sem selecao ===\n";

    geometry::LEM noSelectionMesh;
    geometry::LEMEditor noSelectionEditor(noSelectionMesh);

    geometry::VertexHandle a = noSelectionEditor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
    geometry::VertexHandle b = noSelectionEditor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
    geometry::VertexHandle c = noSelectionEditor.add_vertex(glm::vec3(1.0f, 0.0f, 1.0f));
    geometry::VertexHandle d = noSelectionEditor.add_vertex(glm::vec3(0.0f, 0.0f, 1.0f));

    noSelectionEditor.add_face({ a, b, c, d });
    noSelectionEditor.rebuild_face_normals();
    noSelectionEditor.clear_diff();

    modeling::LoopCutOp selectedOp = modeling::LoopCutOp::selected_edges();

    modeling::OperationContext noSelectionContext;
    noSelectionContext.mesh = &noSelectionMesh;
    noSelectionContext.validateAfterExecute = true;
    noSelectionContext.rebuildNormals = true;
    noSelectionContext.allowNonManifold = true;

    modeling::OperationResult noSelectionResult = selectedOp.execute(noSelectionContext);

    std::cout << "status: " << status_name(noSelectionResult.status()) << '\n';

    if (!noSelectionResult.message().empty()) {
        std::cout << "message: " << noSelectionResult.message() << '\n';
    }

    ok &= expect(
        noSelectionResult.status() == modeling::OperationStatus::NoChange,
        "selected_edges sem selecao retorna NoChange");

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "Todos os testes passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "Algum teste falhou.\n";
    return EXIT_FAILURE;
}