#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/spatial/SpatialIndex.h"
#include "kernel/math/Bounds.h"
#include "kernel/math/Ray.h"

#include <glm/glm.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

	using namespace locus::kernel;
	using namespace locus::kernel::geometry;
	using namespace locus::kernel::math;

	void print_vec3(const glm::vec3& value) {
		std::cout
			<< "("
			<< value.x << ", "
			<< value.y << ", "
			<< value.z
			<< ")";
	}

	void print_bounds(const Bounds& bounds) {
		std::cout << "min=";
		print_vec3(bounds.min);
		std::cout << " max=";
		print_vec3(bounds.max);
	}

	void print_face_handle(FaceHandle handle) {
		if (handle.is_invalid()) {
			std::cout << "invalid";
			return;
		}

		std::cout << handle.id.value;
	}

	void print_hit(const char* label, const SelectionHit& hit) {
		std::cout << "\n" << label << "\n";

		if (!hit.hit) {
			std::cout << "hit: false\n";
			return;
		}

		std::cout << "hit: true\n";
		std::cout << "face: ";
		print_face_handle(hit.face);
		std::cout << "\n";
		std::cout << "distance: " << hit.distance << "\n";
		std::cout << "position: ";
		print_vec3(hit.position);
		std::cout << "\n";
		std::cout << "normal: ";
		print_vec3(hit.normal);
		std::cout << "\n";
	}

	LEM build_test_mesh() {
		LEM mesh;

		const VertexHandle v0 = mesh.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
		const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
		const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
		const VertexHandle v3 = mesh.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });

		const VertexHandle v4 = mesh.add_vertex(glm::vec3{ 2.0f, -0.5f, 0.0f });
		const VertexHandle v5 = mesh.add_vertex(glm::vec3{ 3.0f, -0.5f, 0.0f });
		const VertexHandle v6 = mesh.add_vertex(glm::vec3{ 2.5f, 0.5f, 0.0f });

		mesh.add_face(std::vector<VertexHandle>{ v0, v1, v2, v3 });
		mesh.add_face(std::vector<VertexHandle>{ v4, v5, v6 });

		return mesh;
	}

	bool expect(bool condition, const char* message) {
		if (condition) {
			std::cout << "[OK] " << message << "\n";
			return true;
		}

		std::cout << "[FAIL] " << message << "\n";
		return false;
	}

}

int main() {
	std::cout << std::fixed << std::setprecision(3);

	LEM mesh = build_test_mesh();

	std::cout << "=== Locus3D Kernel BVH Test ===\n\n";

	std::cout << "Mesh\n";
	std::cout << "vertices: " << mesh.vertex_count() << "\n";
	std::cout << "edges: " << mesh.edge_count() << "\n";
	std::cout << "loops: " << mesh.loop_count() << "\n";
	std::cout << "faces: " << mesh.face_count() << "\n\n";

	SpatialIndex spatialIndex;
	spatialIndex.rebuild(mesh);

	const BVH& bvh = spatialIndex.bvh();

	std::cout << "BVH\n";
	std::cout << "valid: " << (bvh.is_valid() ? "true" : "false") << "\n";
	std::cout << "empty: " << (bvh.empty() ? "true" : "false") << "\n";
	std::cout << "nodes: " << bvh.node_count() << "\n";
	std::cout << "triangles: " << bvh.triangle_count() << "\n";
	std::cout << "bounds: ";
	print_bounds(bvh.bounds());
	std::cout << "\n\n";

	bool passed = true;

	passed &= expect(bvh.is_valid(), "BVH valido apos rebuild");
	passed &= expect(!bvh.empty(), "BVH nao esta vazio");
	passed &= expect(bvh.triangle_count() == 3, "quad virou 2 triangulos e triangulo separado virou 1 triangulo");
	passed &= expect(bvh.node_count() > 0, "BVH possui pelo menos um node");

	const Ray centerRay{
		glm::vec3{ 0.0f, 0.0f, 2.0f },
		glm::vec3{ 0.0f, 0.0f, -1.0f }
	};

	const SelectionHit centerHit = spatialIndex.raycast_faces(centerRay);
	print_hit("Raycast centro do quad", centerHit);

	passed &= expect(centerHit.hit, "raycast acertou o quad");
	passed &= expect(centerHit.is_face(), "raycast retornou face");

	const Ray triangleRay{
		glm::vec3{ 2.5f, 0.0f, 2.0f },
		glm::vec3{ 0.0f, 0.0f, -1.0f }
	};

	const SelectionHit triangleHit = spatialIndex.raycast_faces(triangleRay);
	print_hit("Raycast triangulo separado", triangleHit);

	passed &= expect(triangleHit.hit, "raycast acertou o triangulo separado");
	passed &= expect(triangleHit.is_face(), "raycast do triangulo separado retornou face");

	const Ray missRay{
		glm::vec3{ 10.0f, 10.0f, 2.0f },
		glm::vec3{ 0.0f, 0.0f, -1.0f }
	};

	const SelectionHit missHit = spatialIndex.raycast_faces(missRay);
	print_hit("Raycast fora da malha", missHit);

	passed &= expect(!missHit.hit, "raycast fora da malha retornou miss");

	const Bounds quadQueryBounds = Bounds::from_center_size(
		glm::vec3{ 0.0f, 0.0f, 0.0f },
		glm::vec3{ 2.5f, 2.5f, 0.5f }
	);

	const std::vector<FaceHandle> quadBoundsFaces = spatialIndex.query_bounds(quadQueryBounds);

	std::cout << "\nBounds query no quad\n";
	std::cout << "faces encontradas: " << quadBoundsFaces.size() << "\n";
	for (FaceHandle face : quadBoundsFaces) {
		std::cout << "face: ";
		print_face_handle(face);
		std::cout << "\n";
	}

	passed &= expect(!quadBoundsFaces.empty(), "bounds query encontrou pelo menos uma face do quad");
	passed &= expect(spatialIndex.intersects_bounds(quadQueryBounds), "intersects_bounds detectou intersecao com o quad");

	const Bounds emptyAreaBounds = Bounds::from_center_size(
		glm::vec3{ 10.0f, 10.0f, 10.0f },
		glm::vec3{ 1.0f, 1.0f, 1.0f }
	);

	const std::vector<FaceHandle> emptyAreaFaces = spatialIndex.query_bounds(emptyAreaBounds);

	std::cout << "\nBounds query fora da malha\n";
	std::cout << "faces encontradas: " << emptyAreaFaces.size() << "\n";

	passed &= expect(emptyAreaFaces.empty(), "bounds query fora da malha nao encontrou faces");
	passed &= expect(!spatialIndex.intersects_bounds(emptyAreaBounds), "intersects_bounds fora da malha retornou false");

	std::cout << "\nResultado final: " << (passed ? "PASS" : "FAIL") << "\n";

	return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}