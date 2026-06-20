/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/io/IImporter.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace locus::kernel::io {

/**
 * @brief Imports STL triangle meshes into editable LEM meshes.
 */
class StlImporter final : public IImporter {
public:
	/**
	 * @brief Returns the mesh format handled by this importer.
	 *
	 * @return STL mesh format identifier.
	 */
	[[nodiscard]] MeshFormat format() const override
	{
		return MeshFormat::Stl;
	}

	/**
	 * @brief Returns the stable importer display name.
	 *
	 * @return Importer name.
	 */
	[[nodiscard]] std::string_view name() const override
	{
		return "STL Importer";
	}

	/**
	 * @brief Returns the file extensions supported by this importer.
	 *
	 * @return Supported extensions without dot.
	 */
	[[nodiscard]] std::vector<std::string_view> extensions() const override
	{
		return { "stl" };
	}

	/**
	 * @brief Reads an STL file and converts triangles into a LEM mesh.
	 *
	 * @param path Source file path.
	 * @param options Import behavior options.
	 * @return Imported mesh or IO error.
	 */
	[[nodiscard]] Result<geometry::LEM> import_mesh(
		const std::filesystem::path& path,
		const MeshImportOptions& options = {}) const override
	{
		if (path.empty()) {
			return Result<geometry::LEM>::fail(ErrorCode::InvalidArgument, "STL import path is empty.");
		}

		if (!std::filesystem::exists(path)) {
			return Result<geometry::LEM>::fail(ErrorCode::NotFound, "STL import file does not exist.");
		}

		Result<std::vector<Triangle>> parsed = is_binary_stl(path)
			? parse_binary(path)
			: parse_ascii(path);

		if (parsed.is_error()) {
			return Result<geometry::LEM>::fail(parsed.error());
		}

		if (options.requireFaces && parsed.value().empty()) {
			return Result<geometry::LEM>::fail(ErrorCode::DegenerateGeometry, "STL import found no triangles.");
		}

		return build_mesh(parsed.value(), options);
	}

private:
	/**
	 * @brief Triangle position data parsed from STL.
	 */
	struct Triangle {
		/**
		 * @brief First triangle vertex.
		 */
		glm::vec3 a{ 0.0f, 0.0f, 0.0f };

		/**
		 * @brief Second triangle vertex.
		 */
		glm::vec3 b{ 0.0f, 0.0f, 0.0f };

		/**
		 * @brief Third triangle vertex.
		 */
		glm::vec3 c{ 0.0f, 0.0f, 0.0f };
	};

	/**
	 * @brief Checks whether a file has the exact binary STL byte layout.
	 *
	 * @param path File path.
	 * @return True when file size matches binary STL header, count, and triangle records.
	 */
	[[nodiscard]] static bool is_binary_stl(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			return false;
		}

		std::error_code errorCode;
		const std::uintmax_t fileSize = std::filesystem::file_size(path, errorCode);
		if (errorCode || fileSize < 84) {
			return false;
		}

		file.seekg(80, std::ios::beg);

		std::uint32_t triangleCount = 0;
		file.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));

		if (!file) {
			return false;
		}

		const std::uintmax_t expectedSize = 84ull + static_cast<std::uintmax_t>(triangleCount) * 50ull;
		return expectedSize == fileSize;
	}

	/**
	 * @brief Parses a binary STL file.
	 *
	 * @param path Source file path.
	 * @return Parsed triangles or IO error.
	 */
	[[nodiscard]] static Result<std::vector<Triangle>> parse_binary(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			return Result<std::vector<Triangle>>::fail(ErrorCode::IoError, "Failed to open binary STL file for reading.");
		}

		std::array<char, 80> header{};
		file.read(header.data(), static_cast<std::streamsize>(header.size()));

		std::uint32_t triangleCount = 0;
		file.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));

		if (!file) {
			return Result<std::vector<Triangle>>::fail(ErrorCode::IoError, "Failed to read binary STL header.");
		}

		std::vector<Triangle> triangles{};
		triangles.reserve(triangleCount);

		for (std::uint32_t i = 0; i < triangleCount; ++i) {
			glm::vec3 normal{};
			Triangle triangle{};

			if (!read_vec3(file, normal)
				|| !read_vec3(file, triangle.a)
				|| !read_vec3(file, triangle.b)
				|| !read_vec3(file, triangle.c)) {
				return Result<std::vector<Triangle>>::fail(ErrorCode::IoError, "Failed to read binary STL triangle.");
			}

			std::uint16_t attributeByteCount = 0;
			file.read(reinterpret_cast<char*>(&attributeByteCount), sizeof(attributeByteCount));

			if (!file) {
				return Result<std::vector<Triangle>>::fail(ErrorCode::IoError, "Failed to read binary STL attribute bytes.");
			}

			if (is_valid_triangle(triangle)) {
				triangles.push_back(triangle);
			}
		}

		return Result<std::vector<Triangle>>::ok(std::move(triangles));
	}

	/**
	 * @brief Parses an ASCII STL file.
	 *
	 * @param path Source file path.
	 * @return Parsed triangles or IO error.
	 */
	[[nodiscard]] static Result<std::vector<Triangle>> parse_ascii(const std::filesystem::path& path)
	{
		std::ifstream file(path);
		if (!file) {
			return Result<std::vector<Triangle>>::fail(ErrorCode::IoError, "Failed to open ASCII STL file for reading.");
		}

		std::vector<Triangle> triangles{};
		std::string line;
		std::vector<glm::vec3> currentVertices{};
		currentVertices.reserve(3);

		while (std::getline(file, line)) {
			std::istringstream stream(line);

			std::string tag;
			stream >> tag;

			if (tag != "vertex") {
				continue;
			}

			glm::vec3 position{ 0.0f, 0.0f, 0.0f };
			if (!(stream >> position.x >> position.y >> position.z)) {
				return Result<std::vector<Triangle>>::fail(ErrorCode::InvalidArgument, "Invalid ASCII STL vertex record.");
			}

			currentVertices.push_back(position);

			if (currentVertices.size() == 3) {
				Triangle triangle{
					currentVertices[0],
					currentVertices[1],
					currentVertices[2]
				};

				if (is_valid_triangle(triangle)) {
					triangles.push_back(triangle);
				}

				currentVertices.clear();
			}
		}

		if (file.bad()) {
			return Result<std::vector<Triangle>>::fail(ErrorCode::IoError, "Failed while reading ASCII STL file.");
		}

		if (!currentVertices.empty()) {
			return Result<std::vector<Triangle>>::fail(ErrorCode::InvalidArgument, "ASCII STL ended with incomplete triangle.");
		}

		return Result<std::vector<Triangle>>::ok(std::move(triangles));
	}

	/**
	 * @brief Builds a LEM mesh from STL triangles.
	 *
	 * @param triangles Parsed triangle list.
	 * @param options Import behavior options.
	 * @return Imported mesh or conversion error.
	 */
	[[nodiscard]] static Result<geometry::LEM> build_mesh(
		const std::vector<Triangle>& triangles,
		const MeshImportOptions& options)
	{
		geometry::LEM mesh{};
		std::vector<CachedVertex> cache{};

		for (const Triangle& triangle : triangles) {
			const geometry::VertexHandle a = add_or_find_vertex(mesh, cache, triangle.a, options);
			const geometry::VertexHandle b = add_or_find_vertex(mesh, cache, triangle.b, options);
			const geometry::VertexHandle c = add_or_find_vertex(mesh, cache, triangle.c, options);

			if (!a.is_valid() || !b.is_valid() || !c.is_valid()) {
				continue;
			}

			if (a == b || b == c || c == a) {
				continue;
			}

			const geometry::FaceHandle face = mesh.add_face({ a, b, c });
			if (!mesh.is_valid(face) && options.requireFaces) {
				continue;
			}
		}

		if (options.requireFaces && mesh.face_count() == 0) {
			return Result<geometry::LEM>::fail(ErrorCode::DegenerateGeometry, "STL import could not build any valid face.");
		}

		if (options.rebuildNormals) {
			geometry::NormalBuilder::rebuild_face_normals(mesh);
		}

		return Result<geometry::LEM>::ok(std::move(mesh));
	}

	/**
	 * @brief Vertex cache entry used for optional STL vertex welding.
	 */
	struct CachedVertex {
		/**
		 * @brief Vertex position.
		 */
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };

		/**
		 * @brief Handle inserted into the mesh.
		 */
		geometry::VertexHandle handle{};
	};

	/**
	 * @brief Adds a new vertex or reuses an existing one when vertex merging is enabled.
	 *
	 * @param mesh Mesh being built.
	 * @param cache Previously inserted vertices.
	 * @param position Vertex position.
	 * @param options Import behavior options.
	 * @return Vertex handle.
	 */
	[[nodiscard]] static geometry::VertexHandle add_or_find_vertex(
		geometry::LEM& mesh,
		std::vector<CachedVertex>& cache,
		const glm::vec3& position,
		const MeshImportOptions& options)
	{
		if (options.mergeDuplicateVertices) {
			const float epsilonSquared = options.mergeEpsilon * options.mergeEpsilon;

			for (const CachedVertex& cached : cache) {
				const glm::vec3 delta = cached.position - position;
				if (glm::dot(delta, delta) <= epsilonSquared) {
					return cached.handle;
				}
			}
		}

		const geometry::VertexHandle handle = mesh.add_vertex(position);
		cache.push_back(CachedVertex{ position, handle });
		return handle;
	}

	/**
	 * @brief Checks whether a parsed triangle has non-zero area.
	 *
	 * @param triangle Triangle to inspect.
	 * @return True when triangle vertices form a valid area.
	 */
	[[nodiscard]] static bool is_valid_triangle(const Triangle& triangle)
	{
		const glm::vec3 ab = triangle.b - triangle.a;
		const glm::vec3 ac = triangle.c - triangle.a;
		return glm::dot(glm::cross(ab, ac), glm::cross(ab, ac)) > 0.0f;
	}

	/**
	 * @brief Reads a vector from a binary STL stream.
	 *
	 * @param file Input stream.
	 * @param value Output vector.
	 * @return True when all components were read.
	 */
	[[nodiscard]] static bool read_vec3(std::ifstream& file, glm::vec3& value)
	{
		file.read(reinterpret_cast<char*>(&value.x), sizeof(float));
		file.read(reinterpret_cast<char*>(&value.y), sizeof(float));
		file.read(reinterpret_cast<char*>(&value.z), sizeof(float));

		return static_cast<bool>(file);
	}
};

}