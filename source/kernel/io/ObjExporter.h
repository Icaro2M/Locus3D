/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/io/IExporter.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace locus::kernel::io {

/**
 * @brief Exports editable meshes to the Wavefront OBJ mesh format.
 */
class ObjExporter final : public IExporter {
public:
	/**
	 * @brief Returns the mesh format handled by this exporter.
	 *
	 * @return OBJ mesh format identifier.
	 */
	[[nodiscard]] MeshFormat format() const override
	{
		return MeshFormat::Obj;
	}

	/**
	 * @brief Returns the stable exporter display name.
	 *
	 * @return Exporter name.
	 */
	[[nodiscard]] std::string_view name() const override
	{
		return "OBJ Exporter";
	}

	/**
	 * @brief Returns the file extensions supported by this exporter.
	 *
	 * @return Supported extensions without dot.
	 */
	[[nodiscard]] std::vector<std::string_view> extensions() const override
	{
		return { "obj" };
	}

	/**
	 * @brief Writes an editable mesh to an OBJ file.
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
			return Result<void>::fail(ErrorCode::InvalidArgument, "OBJ export path is empty.");
		}

		if (mesh.empty()) {
			return Result<void>::fail(ErrorCode::DegenerateGeometry, "OBJ export received an empty mesh.");
		}

		std::ofstream file(path);
		if (!file) {
			return Result<void>::fail(ErrorCode::IoError, "Failed to open OBJ file for writing.");
		}

		file << std::setprecision(9);
		file << "# Locus3D OBJ export\n";
		file << "o " << object_name(path) << '\n';

		const VertexIndexMap vertexIndices = write_vertices(file, mesh, options);
		const std::size_t faceCount = write_faces(file, mesh, vertexIndices, options);

		if (vertexIndices.empty()) {
			return Result<void>::fail(ErrorCode::DegenerateGeometry, "OBJ export produced no vertices.");
		}

		if (faceCount == 0) {
			return Result<void>::fail(ErrorCode::DegenerateGeometry, "OBJ export produced no faces.");
		}

		if (!file) {
			return Result<void>::fail(ErrorCode::IoError, "Failed while writing OBJ file.");
		}

		return Result<void>::ok();
	}

private:
	using VertexIndexMap = std::unordered_map<std::size_t, std::size_t>;

	/**
	 * @brief Writes active mesh vertices and builds OBJ index mapping.
	 *
	 * @param file Output stream.
	 * @param mesh Mesh to export.
	 * @param options Export behavior options.
	 * @return Mapping from LEM vertex slot to one-based OBJ vertex index.
	 */
	[[nodiscard]] static VertexIndexMap write_vertices(
		std::ofstream& file,
		const geometry::LEM& mesh,
		const MeshExportOptions& options)
	{
		VertexIndexMap vertexIndices{};
		std::size_t nextIndex = 1;

		const std::vector<geometry::Vertex>& vertices = mesh.vertices();

		for (std::size_t i = 0; i < vertices.size(); ++i) {
			const geometry::Vertex& vertex = vertices[i];

			if (options.skipInactiveElements && (vertex.deleted || vertex.hidden)) {
				continue;
			}

			file << "v "
				<< vertex.position.x << ' '
				<< vertex.position.y << ' '
				<< vertex.position.z << '\n';

			vertexIndices.emplace(i, nextIndex);
			++nextIndex;
		}

		return vertexIndices;
	}

	/**
	 * @brief Writes active mesh faces.
	 *
	 * @param file Output stream.
	 * @param mesh Mesh to export.
	 * @param vertexIndices Mapping from LEM vertex slot to one-based OBJ vertex index.
	 * @param options Export behavior options.
	 * @return Number of faces written.
	 */
	[[nodiscard]] static std::size_t write_faces(
		std::ofstream& file,
		const geometry::LEM& mesh,
		const VertexIndexMap& vertexIndices,
		const MeshExportOptions& options)
	{
		std::size_t writtenFaces = 0;
		const std::vector<geometry::Face>& faces = mesh.faces();

		for (std::size_t i = 0; i < faces.size(); ++i) {
			const geometry::Face& face = faces[i];

			if (options.skipInactiveElements && (face.deleted || face.hidden)) {
				continue;
			}

			const geometry::FaceHandle faceHandle{ static_cast<IdValue>(i) };
			if (!mesh.is_valid(faceHandle)) {
				continue;
			}

			const std::vector<std::size_t> faceVertexIndices = collect_face_vertex_indices(
				mesh,
				faceHandle,
				vertexIndices,
				options
			);

			if (faceVertexIndices.size() < 3) {
				continue;
			}

			file << "f";
			for (std::size_t vertexIndex : faceVertexIndices) {
				file << ' ' << vertexIndex;
			}
			file << '\n';

			++writtenFaces;
		}

		return writtenFaces;
	}

	/**
	 * @brief Collects OBJ vertex indices for a mesh face.
	 *
	 * @param mesh Mesh to inspect.
	 * @param faceHandle Face handle.
	 * @param vertexIndices Mapping from LEM vertex slot to one-based OBJ vertex index.
	 * @param options Export behavior options.
	 * @return Ordered OBJ vertex indices for the face.
	 */
	[[nodiscard]] static std::vector<std::size_t> collect_face_vertex_indices(
		const geometry::LEM& mesh,
		geometry::FaceHandle faceHandle,
		const VertexIndexMap& vertexIndices,
		const MeshExportOptions& options)
	{
		std::vector<std::size_t> result{};

		for (geometry::LoopHandle loopHandle : mesh.face_loops(faceHandle)) {
			if (!mesh.is_valid(loopHandle)) {
				return {};
			}

			const geometry::Loop& loop = mesh.loop(loopHandle);

			if (options.skipInactiveElements && loop.deleted) {
				return {};
			}

			if (!mesh.is_valid(loop.vertex)) {
				return {};
			}

			const geometry::Vertex& vertex = mesh.vertex(loop.vertex);

			if (options.skipInactiveElements && (vertex.deleted || vertex.hidden)) {
				return {};
			}

			const std::size_t vertexSlot = static_cast<std::size_t>(loop.vertex.id.value);
			const auto found = vertexIndices.find(vertexSlot);

			if (found == vertexIndices.end()) {
				return {};
			}

			result.push_back(found->second);
		}

		return result;
	}

	/**
	 * @brief Builds a safe OBJ object name from a file path.
	 *
	 * @param path Source file path.
	 * @return Sanitized object name.
	 */
	[[nodiscard]] static std::string object_name(const std::filesystem::path& path)
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
};

}