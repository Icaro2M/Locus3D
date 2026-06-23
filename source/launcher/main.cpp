#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/face/SolidifyOp.h"

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

    void print_mesh_counts(const geometry::LEM& mesh) {
        std::cout
            << "vertices: " << geometry::TopologyTraversal::vertices(mesh).size()
            << " | edges: " << geometry::TopologyTraversal::edges(mesh).size()
            << " | loops: " << geometry::TopologyTraversal::loops(mesh).size()
            << " | faces: " << geometry::TopologyTraversal::faces(mesh).size()
            << '\n';
    }

    bool validate_mesh(const geometry::LEM& mesh) {
        const geometry::TopologyValidationReport report =
            geometry::TopologyValidator::validate(mesh);

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

    std::vector<geometry::VertexHandle> make_quad_vertices(geometry::LEMEditor& editor) {
        std::vector<geometry::VertexHandle> vertices;

        vertices.push_back(editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ 1.0f,  1.0f, 0.0f }));
        vertices.push_back(editor.add_vertex(glm::vec3{ -1.0f,  1.0f, 0.0f }));

        return vertices;
    }

    geometry::FaceHandle make_quad_face(geometry::LEMEditor& editor) {
        return editor.add_face(make_quad_vertices(editor));
    }

    void print_face_normals(const geometry::LEM& mesh) {
        const std::vector<geometry::FaceHandle> faces =
            geometry::TopologyTraversal::faces(mesh);

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

    void test_solidify_single_face_explicit_offset(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: single face explicit offset ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        const geometry::FaceHandle face = make_quad_face(editor);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op(face, glm::vec3{ 0.0f, 0.0f, 1.0f });
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);
        print_face_normals(mesh);

        check(stats, result.is_success(), "solidify por offset explicito terminou com sucesso");
        check(stats, result.changed(), "solidify por offset explicito gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "solidify criou 4 vertices duplicados");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 6, "solidify manteve face original, criou cap e 4 rims");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 24, "solidify criou total de 24 loops");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_solidify_single_face_thickness(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: single face thickness ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        const geometry::FaceHandle face = make_quad_face(editor);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op(face, 0.5f);
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);
        print_face_normals(mesh);

        check(stats, result.is_success(), "solidify por thickness terminou com sucesso");
        check(stats, result.changed(), "solidify por thickness gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "solidify por thickness criou 4 vertices duplicados");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 6, "solidify por thickness criou 6 faces totais");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 24, "solidify por thickness criou 24 loops");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_solidify_selected_face(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: selected face ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        const geometry::FaceHandle face = make_quad_face(editor);
        const bool selected = editor.set_selected(face, true);
        check(stats, selected, "face selecionada para teste");

        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op = modeling::SolidifyOp::selected(glm::vec3{ 0.0f, 0.0f, 1.0f });
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "solidify por face selecionada terminou com sucesso");
        check(stats, result.changed(), "solidify por face selecionada gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "solidify selecionado criou 4 vertices duplicados");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 6, "solidify selecionado criou 6 faces totais");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_solidify_rims_only(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: rims only ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        const geometry::FaceHandle face = make_quad_face(editor);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op(face, glm::vec3{ 0.0f, 0.0f, 1.0f });
        op.set_create_caps(false);
        op.set_create_rims(true);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "solidify rims only terminou com sucesso");
        check(stats, result.changed(), "solidify rims only gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "rims only criou 4 vertices duplicados");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 5, "rims only manteve original e criou 4 rims");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 20, "rims only criou 20 loops totais");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_solidify_caps_only(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: caps only ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        const geometry::FaceHandle face = make_quad_face(editor);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op(face, glm::vec3{ 0.0f, 0.0f, 1.0f });
        op.set_create_caps(true);
        op.set_create_rims(false);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "solidify caps only terminou com sucesso");
        check(stats, result.changed(), "solidify caps only gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "caps only criou 4 vertices duplicados");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 2, "caps only manteve original e criou 1 cap");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 8, "caps only criou 8 loops totais");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_solidify_remove_source_face(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: remove source face ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        const geometry::FaceHandle face = make_quad_face(editor);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op(face, glm::vec3{ 0.0f, 0.0f, 1.0f });
        op.set_keep_source_faces(false);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.is_success(), "solidify removendo origem terminou com sucesso");
        check(stats, result.changed(), "solidify removendo origem gerou diff");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 8, "remove source manteve 8 vertices");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 5, "remove source deixou cap e 4 rims");
        check(stats, geometry::TopologyTraversal::loops(mesh).size() == 20, "remove source deixou 20 loops");
        check(stats, validate_mesh(mesh), "malha final passou na validacao");
    }

    void test_solidify_no_selected_faces(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: no selected faces ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        make_quad_face(editor);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op = modeling::SolidifyOp::selected(glm::vec3{ 0.0f, 0.0f, 1.0f });
        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.status() == modeling::OperationStatus::NoChange, "sem faces selecionadas retornou NoChange");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 4, "sem selecao manteve 4 vertices");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 1, "sem selecao manteve 1 face");
        check(stats, validate_mesh(mesh), "malha continuou valida");
    }

    void test_solidify_disabled_outputs(TestStats& stats) {
        std::cout << "\n=== SolidifyOp: disabled outputs ===\n";

        geometry::LEM mesh;
        geometry::LEMEditor editor(mesh);

        const geometry::FaceHandle face = make_quad_face(editor);
        editor.clear_diff();

        modeling::OperationContext context = make_context(mesh);

        modeling::SolidifyOp op(face, glm::vec3{ 0.0f, 0.0f, 1.0f });
        op.set_create_caps(false);
        op.set_create_rims(false);

        modeling::OperationResult result = op.execute(context);

        print_result(result);
        print_mesh_counts(mesh);

        check(stats, result.status() == modeling::OperationStatus::NoChange, "caps e rims desligados retornou NoChange");
        check(stats, geometry::TopologyTraversal::vertices(mesh).size() == 4, "disabled outputs manteve 4 vertices");
        check(stats, geometry::TopologyTraversal::faces(mesh).size() == 1, "disabled outputs manteve 1 face");
        check(stats, validate_mesh(mesh), "malha continuou valida");
    }

}

int main() {
    std::cout << "=== Locus3D SolidifyOp Regression Test ===\n";

    TestStats stats;

    test_solidify_single_face_explicit_offset(stats);
    test_solidify_single_face_thickness(stats);
    test_solidify_selected_face(stats);
    test_solidify_rims_only(stats);
    test_solidify_caps_only(stats);
    test_solidify_remove_source_face(stats);
    test_solidify_no_selected_faces(stats);
    test_solidify_disabled_outputs(stats);

    std::cout << "\n=== Summary ===\n";
    std::cout << "passed: " << stats.passed << '\n';
    std::cout << "failed: " << stats.failed << '\n';

    if (stats.failed > 0) {
        return 1;
    }

    return 0;
}