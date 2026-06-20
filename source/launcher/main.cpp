
#include "kernel/geometry/primitives/BoxBuilder.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/io/FormatRegistry.h"
#include "kernel/io/StlExporter.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

namespace {

	using namespace locus::kernel;
	using namespace locus::kernel::geometry;
	using namespace locus::kernel::io;

	bool expect(bool condition, const std::string& message)
	{
		if (condition) {
			std::cout << "[OK] " << message << '\n';
			return true;
		}

		std::cout << "[FAIL] " << message << '\n';
		return false;
	}

	std::uint32_t read_binary_stl_triangle_count(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			return 0;
		}

		file.seekg(80, std::ios::beg);

		std::uint32_t triangleCount = 0;
		file.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));

		if (!file) {
			return 0;
		}

		return triangleCount;
	}

	bool file_contains_text(const std::filesystem::path& path, const std::string& text)
	{
		std::ifstream file(path);
		if (!file) {
			return false;
		}

		std::string content(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>()
		);

		return content.find(text) != std::string::npos;
	}

	void print_mesh_summary(const LEM& mesh, const RenderMesh& renderMesh)
	{
		std::cout << "\n=== Mesh summary ===\n";
		std::cout << "LEM vertices:  " << mesh.vertex_count() << '\n';
		std::cout << "LEM edges:     " << mesh.edge_count() << '\n';
		std::cout << "LEM loops:     " << mesh.loop_count() << '\n';
		std::cout << "LEM faces:     " << mesh.face_count() << '\n';
		std::cout << "Render verts:  " << renderMesh.vertex_count() << '\n';
		std::cout << "Triangles:     " << renderMesh.triangle_count() << '\n';
	}

	bool test_registry()
	{
		std::cout << "\n=== FormatRegistry test ===\n";

		bool passed = true;

		FormatRegistry registry;
		auto exporter = std::make_shared<StlExporter>();

		passed &= expect(registry.empty(), "registry começa vazio");
		passed &= expect(registry.register_exporter(exporter), "registrou StlExporter");
		passed &= expect(registry.exporter_count() == 1, "registry possui 1 exporter");
		passed &= expect(registry.can_export(MeshFormat::Stl), "can_export(STL)");
		passed &= expect(registry.can_export_path("model.stl"), "can_export_path(model.stl)");
		passed &= expect(registry.can_export_path("MODEL.STL"), "can_export_path(MODEL.STL)");
		passed &= expect(registry.find_exporter(MeshFormat::Stl) != nullptr, "find_exporter(STL)");
		passed &= expect(registry.find_exporter_for_path("box.stl") != nullptr, "find_exporter_for_path(box.stl)");

		return passed;
	}

	bool test_stl_export()
	{
		std::cout << "\n=== STL export test ===\n";

		bool passed = true;

		BoxParameters parameters;
		parameters.center = { 0.0f, 0.0f, 0.0f };
		parameters.size = { 2.0f, 2.0f, 2.0f };

		LEM mesh = BoxBuilder::build(parameters);
		RenderMesh renderMesh = MeshTriangulator::triangulate(mesh);

		print_mesh_summary(mesh, renderMesh);

		passed &= expect(!mesh.empty(), "BoxBuilder gerou uma LEM não vazia");
		passed &= expect(mesh.vertex_count() == 8, "box possui 8 vertices na LEM");
		passed &= expect(mesh.edge_count() == 12, "box possui 12 edges na LEM");
		passed &= expect(mesh.face_count() == 6, "box possui 6 faces na LEM");
		passed &= expect(renderMesh.triangle_count() == 12, "box triangulada possui 12 triangulos");

		const std::filesystem::path outputDirectory = "stl_test_output";
		const std::filesystem::path asciiPath = outputDirectory / "locus_box_ascii.stl";
		const std::filesystem::path binaryPath = outputDirectory / "locus_box_binary.stl";

		std::error_code errorCode;
		std::filesystem::create_directories(outputDirectory, errorCode);

		passed &= expect(!errorCode, "diretorio de saida criado");

		StlExporter exporter;

		MeshExportOptions asciiOptions;
		asciiOptions.preferBinary = false;
		asciiOptions.triangulateFaces = true;

		MeshExportOptions binaryOptions;
		binaryOptions.preferBinary = true;
		binaryOptions.triangulateFaces = true;

		const Result<void> asciiResult = exporter.export_mesh(mesh, asciiPath, asciiOptions);
		const Result<void> binaryResult = exporter.export_mesh(mesh, binaryPath, binaryOptions);

		if (asciiResult.is_error()) {
			std::cout << "ASCII STL error: " << asciiResult.error().message << '\n';
		}

		if (binaryResult.is_error()) {
			std::cout << "Binary STL error: " << binaryResult.error().message << '\n';
		}

		passed &= expect(asciiResult.is_ok(), "exportou STL ASCII");
		passed &= expect(binaryResult.is_ok(), "exportou STL binario");

		passed &= expect(std::filesystem::exists(asciiPath), "arquivo ASCII existe");
		passed &= expect(std::filesystem::exists(binaryPath), "arquivo binario existe");

		const std::uintmax_t asciiSize = std::filesystem::exists(asciiPath)
			? std::filesystem::file_size(asciiPath)
			: 0;

		const std::uintmax_t binarySize = std::filesystem::exists(binaryPath)
			? std::filesystem::file_size(binaryPath)
			: 0;

		const std::uintmax_t expectedBinarySize = 84 + renderMesh.triangle_count() * 50;

		std::cout << "\n=== Files ===\n";
		std::cout << "ASCII STL:  " << asciiPath.string() << " (" << asciiSize << " bytes)\n";
		std::cout << "Binary STL: " << binaryPath.string() << " (" << binarySize << " bytes)\n";
		std::cout << "Expected binary STL size: " << expectedBinarySize << " bytes\n";

		passed &= expect(asciiSize > 0, "arquivo ASCII nao esta vazio");
		passed &= expect(binarySize == expectedBinarySize, "tamanho do STL binario bate com 84 + triangulos * 50");

		const std::uint32_t binaryTriangleCount = read_binary_stl_triangle_count(binaryPath);

		std::cout << "Binary triangle count read from file: " << binaryTriangleCount << '\n';

		passed &= expect(binaryTriangleCount == renderMesh.triangle_count(), "contador de triangulos no STL binario esta correto");
		passed &= expect(file_contains_text(asciiPath, "solid locus_box_ascii"), "ASCII STL contem solid name");
		passed &= expect(file_contains_text(asciiPath, "facet normal"), "ASCII STL contem facets");
		passed &= expect(file_contains_text(asciiPath, "vertex"), "ASCII STL contem vertices");

		return passed;
	}

}

int main()
{
	std::cout << std::fixed << std::setprecision(3);
	std::cout << "=== Locus3D STL Export Test ===\n";

	bool passed = true;

	passed &= test_registry();
	passed &= test_stl_export();

	std::cout << "\nResultado final: " << (passed ? "PASS" : "FAIL") << '\n';

	return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}