#include "kernel/kernel.h"


#include <glm/glm.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace geometry = locus::kernel::geometry;
namespace modeling = locus::kernel::modeling;

namespace {

	int failures = 0;

	void check(bool condition, const std::string& message)
	{
		if (condition) {
			std::cout << "[OK] " << message << '\n';
			return;
		}

		std::cout << "[FAIL] " << message << '\n';
		++failures;
	}

	void print_mesh_counts(const std::string& label, const geometry::LEM& mesh)
	{
		std::cout << label << '\n';
		std::cout << "  vertices: " << mesh.vertex_count() << '\n';
		std::cout << "  edges:    " << mesh.edge_count() << '\n';
		std::cout << "  loops:    " << mesh.loop_count() << '\n';
		std::cout << "  faces:    " << mesh.face_count() << '\n';
	}

	void print_preview_counts(
		const std::string& label,
		const modeling::OperationPreview& preview)
	{
		const modeling::PreviewMesh& mesh = preview.mesh();

		std::cout << label << '\n';
		std::cout << "  ready:           " << (preview.is_ready() ? "true" : "false") << '\n';
		std::cout << "  empty:           " << (preview.is_empty() ? "true" : "false") << '\n';
		std::cout << "  failure:         " << (preview.is_failure() ? "true" : "false") << '\n';
		std::cout << "  invalidated:     " << (preview.is_invalidated() ? "true" : "false") << '\n';
		std::cout << "  solid vertices:  " << mesh.solid_vertex_count() << '\n';
		std::cout << "  solid triangles: " << mesh.solid_triangle_count() << '\n';
		std::cout << "  wire vertices:   " << mesh.wire_vertex_count() << '\n';
		std::cout << "  wire lines:      " << mesh.wire_line_count() << '\n';
		std::cout << "  changed:         " << (mesh.changed() ? "true" : "false") << '\n';
		std::cout << "  diff size:       " << mesh.diff().size() << '\n';

		if (!preview.message().empty()) {
			std::cout << "  preview message: " << preview.message() << '\n';
		}

		if (!mesh.message().empty()) {
			std::cout << "  mesh message:    " << mesh.message() << '\n';
		}
	}

	geometry::FaceHandle build_quad(geometry::LEM& mesh)
	{
		geometry::LEMEditor editor(mesh);

		const geometry::VertexHandle v0 = editor.add_vertex(glm::vec3{ -1.0f, 0.0f, -1.0f });
		const geometry::VertexHandle v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, -1.0f });
		const geometry::VertexHandle v2 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f,  1.0f });
		const geometry::VertexHandle v3 = editor.add_vertex(glm::vec3{ -1.0f, 0.0f,  1.0f });

		return editor.add_face({ v0, v1, v2, v3 });
	}

	void test_direct_preview()
	{
		std::cout << "\n=== GhostMeshBuilder: preview direto ===\n";

		geometry::LEM mesh;
		const geometry::FaceHandle face = build_quad(mesh);

		check(mesh.is_valid(face), "quad inicial criou face valida");
		check(mesh.vertex_count() == 4, "quad inicial possui 4 vertices");
		check(mesh.edge_count() == 4, "quad inicial possui 4 edges");
		check(mesh.loop_count() == 4, "quad inicial possui 4 loops");
		check(mesh.face_count() == 1, "quad inicial possui 1 face");

		modeling::GhostMeshBuildOptions options;
		options.buildWireframe = true;
		options.normalMode = geometry::NormalBuildMode::Flat;

		const modeling::OperationPreview preview =
			modeling::GhostMeshBuilder::build_preview(mesh, options);

		print_mesh_counts("malha original", mesh);
		print_preview_counts("preview direto", preview);

		check(preview.is_ready(), "preview direto ficou pronto");
		check(preview.mesh().valid(), "preview direto possui payload valido");
		check(!preview.mesh().empty(), "preview direto nao esta vazio");
		check(preview.mesh().solid_triangle_count() == 2, "quad triangulou em 2 triangulos");
		check(preview.mesh().wire_line_count() == 4, "quad gerou 4 linhas de wireframe");
		check(!preview.mesh().changed(), "preview direto nao possui diff de operacao");
	}

	void test_operation_preview()
	{
		std::cout << "\n=== GhostMeshBuilder: preview com ExtrudeFaceOp ===\n";

		geometry::LEM sourceMesh;
		const geometry::FaceHandle face = build_quad(sourceMesh);

		check(sourceMesh.is_valid(face), "malha fonte criou face valida");

		const std::size_t sourceVertexCount = sourceMesh.vertex_count();
		const std::size_t sourceEdgeCount = sourceMesh.edge_count();
		const std::size_t sourceLoopCount = sourceMesh.loop_count();
		const std::size_t sourceFaceCount = sourceMesh.face_count();

		modeling::ExtrudeFaceOp operation(face, glm::vec3{ 0.0f, 1.0f, 0.0f });
		operation.set_keep_source_face(false);

		modeling::GhostMeshBuildOptions options;
		options.buildWireframe = true;
		options.normalMode = geometry::NormalBuildMode::Flat;
		options.validateAfterPreview = true;
		options.rebuildNormals = true;
		options.allowNonManifold = true;

		const modeling::OperationPreview preview =
			modeling::GhostMeshBuilder::build_operation_preview(
				sourceMesh,
				operation,
				options
			);

		print_mesh_counts("malha fonte apos preview", sourceMesh);
		print_preview_counts("preview extrude", preview);

		check(preview.is_ready(), "preview da extrusao ficou pronto");
		check(!preview.is_failure(), "preview da extrusao nao falhou");
		check(preview.mesh().valid(), "preview da extrusao possui payload valido");
		check(!preview.mesh().empty(), "preview da extrusao nao esta vazio");
		check(preview.mesh().changed(), "preview da extrusao possui diff");
		check(preview.mesh().diff().size() > 0, "preview da extrusao registrou alteracoes no diff");

		check(
			preview.mesh().solid_triangle_count() == 10,
			"extrusao de quad sem face fonte gerou 10 triangulos"
		);

		check(
			preview.mesh().wire_line_count() == 12,
			"extrusao de quad sem face fonte gerou 12 linhas de wireframe"
		);

		check(
			sourceMesh.vertex_count() == sourceVertexCount,
			"preview preservou quantidade de vertices da malha fonte"
		);

		check(
			sourceMesh.edge_count() == sourceEdgeCount,
			"preview preservou quantidade de edges da malha fonte"
		);

		check(
			sourceMesh.loop_count() == sourceLoopCount,
			"preview preservou quantidade de loops da malha fonte"
		);

		check(
			sourceMesh.face_count() == sourceFaceCount,
			"preview preservou quantidade de faces da malha fonte"
		);
	}

	void test_failed_operation_preview()
	{
		std::cout << "\n=== GhostMeshBuilder: preview com operacao sem alvo valido ===\n";

		geometry::LEM sourceMesh;
		build_quad(sourceMesh);

		modeling::ExtrudeFaceOp operation;
		operation.set_faces({ geometry::FaceHandle{} });

		modeling::GhostMeshBuildOptions options;
		options.buildWireframe = true;
		options.validateAfterPreview = true;

		const modeling::OperationPreview preview =
			modeling::GhostMeshBuilder::build_operation_preview(
				sourceMesh,
				operation,
				options
			);

		print_preview_counts("preview sem alvo valido", preview);

		check(preview.is_empty(), "operacao sem face valida retornou preview vazio");
		check(!preview.is_failure(), "operacao sem face valida nao e falha fatal");
		check(!preview.message().empty(), "preview vazio trouxe mensagem de no-change");
	}

}

int main()
{
	std::cout << "=== Locus3D Operation Preview Regression Test ===\n";

	test_direct_preview();
	test_operation_preview();
	test_failed_operation_preview();

	std::cout << "\n=== Resultado ===\n";

	if (failures == 0) {
		std::cout << "Todos os testes passaram.\n";
		return EXIT_SUCCESS;
	}

	std::cout << failures << " teste(s) falharam.\n";
	return EXIT_FAILURE;
}