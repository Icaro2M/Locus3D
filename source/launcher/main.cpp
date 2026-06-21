#include "kernel/geometry/mesh/LEM.h"
#include "kernel/validation/checks/lem/ElementReferenceCheck.h"
#include "kernel/validation/checks/lem/FaceCycleCheck.h"
#include "kernel/validation/checks/lem/HandleValidityCheck.h"
#include "kernel/validation/checks/lem/RadialCycleCheck.h"
#include "kernel/validation/core/ValidationReport.h"
#include "kernel/validation/pipeline/ValidationPipeline.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

namespace {

    using namespace locus::kernel;
    using namespace locus::kernel::geometry;
    using namespace locus::kernel::validation;

    bool expect(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << "\n";
            return true;
        }

        std::cout << "[FAIL] " << message << "\n";
        return false;
    }

    const char* severity_name(ValidationSeverity severity)
    {
        switch (severity) {
        case ValidationSeverity::Info:
            return "Info";
        case ValidationSeverity::Warning:
            return "Warning";
        case ValidationSeverity::Error:
            return "Error";
        }

        return "Unknown";
    }

    void print_report(const ValidationReport& report)
    {
        std::cout << "issues: " << report.issue_count()
            << " | errors: " << report.error_count()
            << " | warnings: " << report.warning_count()
            << " | info: " << report.info_count()
            << "\n";

        for (const ValidationIssue& issue : report.issues()) {
            std::cout << "  [" << severity_name(issue.severity) << "] "
                << issue.code
                << " | check=" << issue.checkName
                << " | target=" << issue.targetType
                << " | id=";

            if (issue.targetId.is_valid()) {
                std::cout << issue.targetId.value;
            }
            else {
                std::cout << "invalid";
            }

            std::cout << " | " << issue.message << "\n";
        }
    }

    ValidationPipeline make_pipeline()
    {
        ValidationPipeline pipeline{};
        pipeline.add_check(std::make_unique<HandleValidityCheck>());
        pipeline.add_check(std::make_unique<ElementReferenceCheck>());
        pipeline.add_check(std::make_unique<FaceCycleCheck>());
        pipeline.add_check(std::make_unique<RadialCycleCheck>());
        return pipeline;
    }

    LEM make_quad()
    {
        LEM mesh{};

        const VertexHandle v0 = mesh.add_vertex(glm::vec3{ -1.0f, 0.0f, -1.0f });
        const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, -1.0f });
        const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 1.0f });
        const VertexHandle v3 = mesh.add_vertex(glm::vec3{ -1.0f, 0.0f, 1.0f });

        const FaceHandle face = mesh.add_face({ v0, v1, v2, v3 });

        expect(face.is_valid(), "quad criado com face valida");
        expect(mesh.vertex_count() == 4, "quad possui 4 vertices");
        expect(mesh.edge_count() == 4, "quad possui 4 edges");
        expect(mesh.loop_count() == 4, "quad possui 4 loops");
        expect(mesh.face_count() == 1, "quad possui 1 face");

        return mesh;
    }

    LEM make_two_quads_sharing_edge()
    {
        LEM mesh{};

        const VertexHandle v0 = mesh.add_vertex(glm::vec3{ -1.0f, 0.0f, -1.0f });
        const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, -1.0f });
        const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 1.0f });
        const VertexHandle v3 = mesh.add_vertex(glm::vec3{ -1.0f, 0.0f, 1.0f });
        const VertexHandle v4 = mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, -1.0f });
        const VertexHandle v5 = mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 1.0f });

        const FaceHandle left = mesh.add_face({ v0, v1, v2, v3 });
        const FaceHandle right = mesh.add_face({ v1, v4, v5, v2 });

        expect(left.is_valid(), "primeiro quad criado");
        expect(right.is_valid(), "segundo quad criado");
        expect(mesh.vertex_count() == 6, "malha compartilhada possui 6 vertices");
        expect(mesh.edge_count() == 7, "malha compartilhada possui 7 edges");
        expect(mesh.loop_count() == 8, "malha compartilhada possui 8 loops");
        expect(mesh.face_count() == 2, "malha compartilhada possui 2 faces");

        return mesh;
    }

    bool test_valid_quad()
    {
        std::cout << "\n=== Validacao: quad valido ===\n";

        const ValidationPipeline pipeline = make_pipeline();
        LEM mesh = make_quad();

        const ValidationReport report = pipeline.validate(ValidationContext{ &mesh });
        print_report(report);

        return expect(report.valid(), "quad valido nao gera erro");
    }

    bool test_valid_shared_edge()
    {
        std::cout << "\n=== Validacao: dois quads compartilhando edge ===\n";

        const ValidationPipeline pipeline = make_pipeline();
        LEM mesh = make_two_quads_sharing_edge();

        const ValidationReport report = pipeline.validate(ValidationContext{ &mesh });
        print_report(report);

        return expect(report.valid(), "radial cycle com edge compartilhada permanece valido");
    }

    bool test_invalid_edge_vertex()
    {
        std::cout << "\n=== Validacao: edge com vertex invalido ===\n";

        const ValidationPipeline pipeline = make_pipeline();
        LEM mesh = make_quad();

        mesh.edge(EdgeHandle(0)).vertexA = VertexHandle(999);

        const ValidationReport report = pipeline.validate(ValidationContext{ &mesh });
        print_report(report);

        return expect(report.has_errors(), "edge com vertex invalido gera erro");
    }

    bool test_broken_face_cycle()
    {
        std::cout << "\n=== Validacao: ciclo de face quebrado ===\n";

        const ValidationPipeline pipeline = make_pipeline();
        LEM mesh = make_quad();

        mesh.loop(LoopHandle(0)).next = LoopHandle(2);

        const ValidationReport report = pipeline.validate(ValidationContext{ &mesh });
        print_report(report);

        return expect(report.has_errors(), "face cycle quebrado gera erro");
    }

    bool test_broken_radial_cycle()
    {
        std::cout << "\n=== Validacao: ciclo radial quebrado ===\n";

        const ValidationPipeline pipeline = make_pipeline();
        LEM mesh = make_two_quads_sharing_edge();

        EdgeHandle sharedEdge{};
        for (std::size_t index = 0; index < mesh.edge_count(); ++index) {
            const EdgeHandle edgeHandle(static_cast<IdValue>(index));
            const Edge& edge = mesh.edge(edgeHandle);

            if (!mesh.is_valid(edge.loop)) {
                continue;
            }

            const LoopHandle first = edge.loop;
            const LoopHandle next = mesh.loop(first).radialNext;

            if (next != first) {
                sharedEdge = edgeHandle;
                break;
            }
        }

        expect(sharedEdge.is_valid(), "edge compartilhada encontrada para teste radial");

        const LoopHandle entry = mesh.edge(sharedEdge).loop;
        mesh.loop(entry).radialNext = LoopHandle(999);

        const ValidationReport report = pipeline.validate(ValidationContext{ &mesh });
        print_report(report);

        return expect(report.has_errors(), "radial cycle quebrado gera erro");
    }

    bool test_pipeline_without_mesh()
    {
        std::cout << "\n=== Validacao: contexto sem mesh ===\n";

        const ValidationPipeline pipeline = make_pipeline();

        const ValidationReport report = pipeline.validate(ValidationContext{});
        print_report(report);

        return expect(report.has_errors(), "contexto sem mesh gera erro");
    }

}

int main()
{
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "=== Locus3D LEM Validation Test ===\n";

    bool passed = true;

    passed &= test_valid_quad();
    passed &= test_valid_shared_edge();
    passed &= test_invalid_edge_vertex();
    passed &= test_broken_face_cycle();
    passed &= test_broken_radial_cycle();
    passed &= test_pipeline_without_mesh();

    std::cout << "\nResultado final: " << (passed ? "PASS" : "FAIL") << "\n";

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}