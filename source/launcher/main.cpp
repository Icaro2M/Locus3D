#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/edge/BevelOp.h"

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

        if (vertex.selected) {
            std::cout << " | selected";
        }

        std::cout << '\n';
    }
}

static void print_edges(const geometry::LEM& mesh)
{
    const auto edges = geometry::TopologyTraversal::edges(mesh);

    std::cout << "\n=== Edges ===\n";

    for (geometry::EdgeHandle edgeHandle : edges) {
        const geometry::Edge& edge = mesh.edge(edgeHandle);

        std::cout << "e" << edgeHandle.id.value
            << ": v" << edge.vertexA.id.value
            << " -> v" << edge.vertexB.id.value;

        if (edge.selected) {
            std::cout << " | selected";
        }

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
    std::cout << "=== Locus3D BevelOp Regression Test ===\n\n";

    bool ok = true;

    geometry::LEM mesh;
    geometry::LEMEditor editor(mesh);

    geometry::VertexHandle v0 = editor.add_vertex(glm::vec3(-1.0f, 0.0f, -1.0f));
    geometry::VertexHandle v1 = editor.add_vertex(glm::vec3(1.0f, 0.0f, -1.0f));
    geometry::VertexHandle v2 = editor.add_vertex(glm::vec3(1.0f, 0.0f, 1.0f));
    geometry::VertexHandle v3 = editor.add_vertex(glm::vec3(-1.0f, 0.0f, 1.0f));

    geometry::FaceHandle face = editor.add_face({ v0, v1, v2, v3 });
    editor.rebuild_face_normals();

    geometry::EdgeHandle targetEdge = mesh.find_edge(v0, v1);

    ok &= expect(mesh.is_valid(v0), "v0 valido");
    ok &= expect(mesh.is_valid(v1), "v1 valido");
    ok &= expect(mesh.is_valid(v2), "v2 valido");
    ok &= expect(mesh.is_valid(v3), "v3 valido");
    ok &= expect(mesh.is_valid(face), "quad criado");
    ok &= expect(mesh.is_valid(targetEdge), "edge alvo encontrada");

    editor.clear_diff();

    std::cout << "\n=== Antes do BevelOp ===\n";
    print_counts(mesh);
    print_vertices(mesh);
    print_edges(mesh);
    print_faces(mesh);

    geometry::TopologyValidationReport beforeReport =
        geometry::TopologyValidator::validate(mesh);

    std::cout << "\n=== Validacao antes ===\n";
    print_validation_report(beforeReport);
    ok &= expect(beforeReport.valid(), "malha inicial valida");

    modeling::BevelOp op(targetEdge, 0.25f);

    modeling::OperationContext context;
    context.mesh = &mesh;
    context.validateAfterExecute = true;
    context.rebuildNormals = true;
    context.allowNonManifold = true;

    modeling::OperationResult result = op.execute(context);

    std::cout << "\n=== Resultado do BevelOp por edge explicita ===\n";
    std::cout << "status: " << status_name(result.status()) << '\n';

    if (!result.message().empty()) {
        std::cout << "message: " << result.message() << '\n';
    }

    ok &= expect(result.is_success(), "BevelOp terminou com sucesso");
    ok &= expect(result.changed(), "BevelOp gerou diff");

    std::cout << "\n=== Depois do BevelOp ===\n";
    print_counts(mesh);
    print_vertices(mesh);
    print_edges(mesh);
    print_faces(mesh);

    const auto verticesAfter = geometry::TopologyTraversal::vertices(mesh);
    const auto edgesAfter = geometry::TopologyTraversal::edges(mesh);
    const auto loopsAfter = geometry::TopologyTraversal::loops(mesh);
    const auto facesAfter = geometry::TopologyTraversal::faces(mesh);

    ok &= expect(verticesAfter.size() == 8, "bevel criou 4 vertices novos");
    ok &= expect(facesAfter.size() == 3, "bevel criou face principal e 2 faces de chanfro");
    ok &= expect(edgesAfter.size() >= 8, "bevel criou novas edges");
    ok &= expect(loopsAfter.size() >= 10, "bevel criou novos loops");

    geometry::TopologyValidationReport afterReport =
        geometry::TopologyValidator::validate(mesh);

    std::cout << "\n=== Validacao depois ===\n";
    print_validation_report(afterReport);
    ok &= expect(afterReport.valid(), "malha final valida");

    std::cout << "\n=== Teste com largura zero ===\n";

    modeling::BevelOp zeroOp(targetEdge, 0.0f);

    modeling::OperationContext zeroContext;
    zeroContext.mesh = &mesh;
    zeroContext.validateAfterExecute = true;
    zeroContext.rebuildNormals = true;
    zeroContext.allowNonManifold = true;

    modeling::OperationResult zeroResult = zeroOp.execute(zeroContext);

    std::cout << "status: " << status_name(zeroResult.status()) << '\n';

    if (!zeroResult.message().empty()) {
        std::cout << "message: " << zeroResult.message() << '\n';
    }

    ok &= expect(
        zeroResult.status() == modeling::OperationStatus::NoChange,
        "largura zero retorna NoChange");

    std::cout << "\n=== Teste selected_edges sem selecao ===\n";

    geometry::LEM noSelectionMesh;
    geometry::LEMEditor noSelectionEditor(noSelectionMesh);

    geometry::VertexHandle a = noSelectionEditor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
    geometry::VertexHandle b = noSelectionEditor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
    geometry::VertexHandle c = noSelectionEditor.add_vertex(glm::vec3(1.0f, 0.0f, 1.0f));
    geometry::VertexHandle d = noSelectionEditor.add_vertex(glm::vec3(0.0f, 0.0f, 1.0f));

    noSelectionEditor.add_face({ a, b, c, d });
    noSelectionEditor.rebuild_face_normals();
    noSelectionEditor.clear_diff();

    modeling::BevelOp selectedOp = modeling::BevelOp::selected_edges(0.25f);

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