/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/io/IExporter.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace locus::kernel::io {

/**
 * @brief Exports editable meshes to the STL triangle mesh format.
 */
class StlExporter final : public IExporter {
public:
	/**
	 * @brief Returns the mesh format handled by this exporter.
	 *
	 * @return STL mesh format identifier.
	 */
	[[nodiscard]] MeshFormat format() const override
	{
		return MeshFormat::Stl;
	}

	/**
	 * @brief Returns the stable exporter display name.
	 *
	 * @return Exporter name.
	 */
	[[nodiscard]] std::string_view name() const override
	{
		return "STL Exporter";
	}

	/**
	 * @brief Returns the file extensions supported by this exporter.
	 *
	 * @return Supported extensions without dot.
	 */
	[[nodiscard]] std::vector<std::string_view> extensions() const override
	{
		return { "stl" };
	}

	/**
	 * @brief Writes an editable mesh to an STL file.
	 *
	 * @param mesh Mesh to export.
	 * @param path Destination file path.
	 * @param options Export behavior options.
	 * @return Success or IO error.
	 */
	[[nodiscard]] Result<void> export_mesh(
		const geometry::LEM& mesh,
		const std::filesystem::path& path,
		const MeshExportOptions& options = {}) const override
	{
		if (path.empty()) {
			return Result<void>::fail(ErrorCode::InvalidArgument, "STL export path is empty.");
		}

		if (!options.triangulateFaces) {
			return Result<void>::fail(ErrorCode::UnsupportedOperation, "STL export requires triangulation.");
		}

		geometry::RenderMesh renderMesh = geometry::MeshTriangulator::triangulate(mesh);

		if (renderMesh.triangle_count() == 0) {
			return Result<void>::fail(ErrorCode::DegenerateGeometry, "STL export produced no triangles.");
		}

		if (renderMesh.triangle_count() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
			return Result<void>::fail(ErrorCode::OutOfRange, "STL triangle count exceeds binary STL limits.");
		}

		if (options.preferBinary) {
			return export_binary(renderMesh, path);
		}

		return export_ascii(renderMesh, path);
	}

private:
	/**
	 * @brief Writes a render mesh as ASCII STL.
	 *
	 * @param mesh Triangulated render mesh.
	 * @param path Destination file path.
	 * @return Success or IO error.
	 */
	[[nodiscard]] static Result<void> export_ascii(
		const geometry::RenderMesh& mesh,
		const std::filesystem::path& path)
	{
		std::ofstream file(path);
		if (!file) {
			return Result<void>::fail(ErrorCode::IoError, "Failed to open ASCII STL file for writing.");
		}

		file << std::setprecision(9);
		file << "solid " << solid_name(path) << '\n';

		for (const geometry::RenderTriangle& triangle : mesh.triangles) {
			if (!is_valid_triangle(mesh, triangle)) {
				continue;
			}

			const glm::vec3& a = mesh.vertices[triangle.a].position;
			const glm::vec3& b = mesh.vertices[triangle.b].position;
			const glm::vec3& c = mesh.vertices[triangle.c].position;
			const glm::vec3 normal = geometry::NormalBuilder::triangle_normal(a, b, c);

			file << "  facet normal " << normal.x << ' ' << normal.y << ' ' << normal.z << '\n';
			file << "    outer loop\n";
			file << "      vertex " << a.x << ' ' << a.y << ' ' << a.z << '\n';
			file << "      vertex " << b.x << ' ' << b.y << ' ' << b.z << '\n';
			file << "      vertex " << c.x << ' ' << c.y << ' ' << c.z << '\n';
			file << "    endloop\n";
			file << "  endfacet\n";
		}

		file << "endsolid " << solid_name(path) << '\n';

		if (!file) {
			return Result<void>::fail(ErrorCode::IoError, "Failed while writing ASCII STL file.");
		}

		return Result<void>::ok();
	}

	/**
	 * @brief Writes a render mesh as binary STL.
	 *
	 * @param mesh Triangulated render mesh.
	 * @param path Destination file path.
	 * @return Success or IO error.
	 */
	[[nodiscard]] static Result<void> export_binary(
		const geometry::RenderMesh& mesh,
		const std::filesystem::path& path)
	{
		std::ofstream file(path, std::ios::binary);
		if (!file) {
			return Result<void>::fail(ErrorCode::IoError, "Failed to open binary STL file for writing.");
		}

		std::array<char, 80> header{};
		const std::string label = "Locus3D binary STL";
		for (std::size_t i = 0; i < label.size() && i < header.size(); ++i) {
			header[i] = label[i];
		}

		const std::uint32_t triangleCount = static_cast<std::uint32_t>(valid_triangle_count(mesh));

		file.write(header.data(), static_cast<std::streamsize>(header.size()));
		write_u32(file, triangleCount);

		for (const geometry::RenderTriangle& triangle : mesh.triangles) {
			if (!is_valid_triangle(mesh, triangle)) {
				continue;
			}

			const glm::vec3& a = mesh.vertices[triangle.a].position;
			const glm::vec3& b = mesh.vertices[triangle.b].position;
			const glm::vec3& c = mesh.vertices[triangle.c].position;
			const glm::vec3 normal = geometry::NormalBuilder::triangle_normal(a, b, c);

			write_vec3(file, normal);
			write_vec3(file, a);
			write_vec3(file, b);
			write_vec3(file, c);
			write_u16(file, 0);
		}

		if (!file) {
			return Result<void>::fail(ErrorCode::IoError, "Failed while writing binary STL file.");
		}

		return Result<void>::ok();
	}

	/**
	 * @brief Checks whether a render triangle references valid vertex indices.
	 *
	 * @param mesh Render mesh containing the triangle.
	 * @param triangle Triangle to inspect.
	 * @return True when all referenced vertices exist and indices are distinct.
	 */
	[[nodiscard]] static bool is_valid_triangle(
		const geometry::RenderMesh& mesh,
		const geometry::RenderTriangle& triangle)
	{
		return triangle.a < mesh.vertices.size()
			&& triangle.b < mesh.vertices.size()
			&& triangle.c < mesh.vertices.size()
			&& triangle.a != triangle.b
			&& triangle.b != triangle.c
			&& triangle.c != triangle.a;
	}

	/**
	 * @brief Counts triangles that can be safely written.
	 *
	 * @param mesh Render mesh to inspect.
	 * @return Number of valid triangles.
	 */
	[[nodiscard]] static std::size_t valid_triangle_count(const geometry::RenderMesh& mesh)
	{
		std::size_t count = 0;

		for (const geometry::RenderTriangle& triangle : mesh.triangles) {
			if (is_valid_triangle(mesh, triangle)) {
				++count;
			}
		}

		return count;
	}

	/**
	 * @brief Builds a safe ASCII STL solid name from a file path.
	 *
	 * @param path Source file path.
	 * @return Sanitized solid name.
	 */
	[[nodiscard]] static std::string solid_name(const std::filesystem::path& path)
	{
		std::string name = path.stem().string();

		if (name.empty()) {
			return "locus3d_mesh";
		}

		for (char& character : name) {
			if (character == '\n' || character == '\r' || character == '\t' || character == ' ') {
				character = '_';
			}
		}

		return name;
	}

	/**
	 * @brief Writes a 32-bit unsigned integer in the host binary representation.
	 *
	 * @param file Output stream.
	 * @param value Value to write.
	 */
	static void write_u32(std::ofstream& file, std::uint32_t value)
	{
		file.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	/**
	 * @brief Writes a 16-bit unsigned integer in the host binary representation.
	 *
	 * @param file Output stream.
	 * @param value Value to write.
	 */
	static void write_u16(std::ofstream& file, std::uint16_t value)
	{
		file.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	/**
	 * @brief Writes a 32-bit floating-point value in the host binary representation.
	 *
	 * @param file Output stream.
	 * @param value Value to write.
	 */
	static void write_f32(std::ofstream& file, float value)
	{
		file.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	/**
	 * @brief Writes a vector as three 32-bit floating-point values.
	 *
	 * @param file Output stream.
	 * @param value Vector to write.
	 */
	static void write_vec3(std::ofstream& file, const glm::vec3& value)
	{
		write_f32(file, value.x);
		write_f32(file, value.y);
		write_f32(file, value.z);
	}
};

}