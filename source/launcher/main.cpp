/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/primitives/BoxBuilder.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"
#include "kernel/io/FormatRegistry.h"
#include "kernel/io/ObjExporter.h"

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

	std::size_t count_lines_starting_with(const std::filesystem::path& path, const std::string& prefix)
	{
		std::ifstream file(path);
		if (!file) {
			return 0;
		}

		std::size_t count = 0;
		std::string line;

		while (std::getline(file, line)) {
			if (line.rfind(prefix, 0) == 0) {
				++count;
			}
		}

		return count;
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

	void print_file_preview(const std::filesystem::path& path, std::size_t maxLines = 16)
	{
		std::ifstream file(path);
		if (!file) {
			std::cout << "Nao foi possivel abrir preview do arquivo.\n";
			return;
		}

		std::cout << "\n=== OBJ preview ===\n";

		std::string line;
		std::size_t lineCount = 0;

		while (lineCount < maxLines && std::getline(file, line)) {
			std::cout << line << '\n';
			++lineCount;
		}

		if (file.good()) {
			std::cout << "...\n";
		}
	}

	void print_mesh_summary(const LEM& mesh)
	{
		std::cout << "\n=== Mesh summary ===\n";
		std::cout << "LEM vertices: " << mesh.vertex_count() << '\n';
		std::cout << "LEM edges:    " << mesh.edge_count() << '\n';
		std::cout << "LEM loops:    " << mesh.loop_count() << '\n';
		std::cout << "LEM faces:    " << mesh.face_count() << '\n';
	}

	bool test_registry()
	{
		std::cout << "\n=== FormatRegistry test ===\n";

		bool passed = true;

		FormatRegistry registry;
		auto exporter = std::make_shared<ObjExporter>();

		passed &= expect(registry.empty(), "registry começa vazio");
		passed &= expect(registry.register_exporter(exporter), "registrou ObjExporter");
		passed &= expect(registry.exporter_count() == 1, "registry possui 1 exporter");
		passed &= expect(registry.can_export(MeshFormat::Obj), "can_export(OBJ)");
		passed &= expect(registry.can_export_path("model.obj"), "can_export_path(model.obj)");
		passed &= expect(registry.can_export_path("MODEL.OBJ"), "can_export_path(MODEL.OBJ)");
		passed &= expect(registry.find_exporter(MeshFormat::Obj) != nullptr, "find_exporter(OBJ)");
		passed &= expect(registry.find_exporter_for_path("box.obj") != nullptr, "find_exporter_for_path(box.obj)");

		return passed;
	}

	bool test_obj_export()
	{
		std::cout << "\n=== OBJ export test ===\n";

		bool passed = true;

		BoxParameters parameters;
		parameters.center = { 0.0f, 0.0f, 0.0f };
		parameters.size = { 2.0f, 2.0f, 2.0f };

		LEM mesh = BoxBuilder::build(parameters);

		print_mesh_summary(mesh);

		passed &= expect(!mesh.empty(), "BoxBuilder gerou uma LEM não vazia");
		passed &= expect(mesh.vertex_count() == 8, "box possui 8 vertices na LEM");
		passed &= expect(mesh.edge_count() == 12, "box possui 12 edges na LEM");
		passed &= expect(mesh.loop_count() == 24, "box possui 24 loops na LEM");
		passed &= expect(mesh.face_count() == 6, "box possui 6 faces na LEM");

		const std::filesystem::path outputDirectory = "obj_test_output";
		const std::filesystem::path objPath = outputDirectory / "locus_box.obj";

		std::error_code errorCode;
		std::filesystem::create_directories(outputDirectory, errorCode);

		passed &= expect(!errorCode, "diretorio de saida criado");

		ObjExporter exporter;

		MeshExportOptions options;
		options.skipInactiveElements = true;
		options.triangulateFaces = false;
		options.writeNormals = false;
		options.preferBinary = false;

		const Result<void> exportResult = exporter.export_mesh(mesh, objPath, options);

		if (exportResult.is_error()) {
			std::cout << "OBJ export error: " << exportResult.error().message << '\n';
		}

		passed &= expect(exportResult.is_ok(), "exportou OBJ");
		passed &= expect(std::filesystem::exists(objPath), "arquivo OBJ existe");

		const std::uintmax_t objSize = std::filesystem::exists(objPath)
			? std::filesystem::file_size(objPath)
			: 0;

		const std::size_t vertexLineCount = count_lines_starting_with(objPath, "v ");
		const std::size_t faceLineCount = count_lines_starting_with(objPath, "f ");

		std::cout << "\n=== File ===\n";
		std::cout << "OBJ: " << objPath.string() << " (" << objSize << " bytes)\n";
		std::cout << "Vertex lines: " << vertexLineCount << '\n';
		std::cout << "Face lines:   " << faceLineCount << '\n';

		passed &= expect(objSize > 0, "arquivo OBJ nao esta vazio");
		passed &= expect(vertexLineCount == 8, "OBJ possui 8 linhas de vertices");
		passed &= expect(faceLineCount == 6, "OBJ possui 6 linhas de faces");
		passed &= expect(file_contains_text(objPath, "# Locus3D OBJ export"), "OBJ contem cabecalho Locus3D");
		passed &= expect(file_contains_text(objPath, "o locus_box"), "OBJ contem nome do objeto");
		passed &= expect(file_contains_text(objPath, "f "), "OBJ contem faces");

		print_file_preview(objPath);

		return passed;
	}

}

int main()
{
	std::cout << std::fixed << std::setprecision(3);
	std::cout << "=== Locus3D OBJ Export Test ===\n";

	bool passed = true;

	passed &= test_registry();
	passed &= test_obj_export();

	std::cout << "\nResultado final: " << (passed ? "PASS" : "FAIL") << '\n';

	return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}