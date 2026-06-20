/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/io/IImporter.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace locus::kernel::io {

/**
 * @brief Imports Wavefront OBJ polygon meshes into editable LEM meshes.
 */
class ObjImporter final : public IImporter {
public:
	/**
	 * @brief Returns the mesh format handled by this importer.
	 *
	 * @return OBJ mesh format identifier.
	 */
	[[nodiscard]] MeshFormat format() const override
	{
		return MeshFormat::Obj;
	}

	/**
	 * @brief Returns the stable importer display name.
	 *
	 * @return Importer name.
	 */
	[[nodiscard]] std::string_view name() const override
	{
		return "OBJ Importer";
	}

	/**
	 * @brief Returns the file extensions supported by this importer.
	 *
	 * @return Supported extensions without dot.
	 */
	[[nodiscard]] std::vector<std::string_view> extensions() const override
	{
		return { "obj" };
	}

	/**
	 * @brief Reads an OBJ file and converts supported polygonal faces into a LEM mesh.
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
			return Result<geometry::LEM>::fail(ErrorCode::InvalidArgument, "OBJ import path is empty.");
		}

		std::ifstream file(path);
		if (!file) {
			return Result<geometry::LEM>::fail(ErrorCode::IoError, "Failed to open OBJ file for reading.");
		}

		ParsedObj parsed{};
		std::string line;
		std::size_t lineNumber = 0;

		while (std::getline(file, line)) {
			++lineNumber;

			trim(line);
			if (line.empty() || line.front() == '#') {
				continue;
			}

			std::istringstream stream(line);
			std::string tag;
			stream >> tag;

			if (tag == "v") {
				const Result<void> result = parse_vertex(stream, parsed.positions);
				if (result.is_error()) {
					return Result<geometry::LEM>::fail(
						result.error().code,
						"Invalid OBJ vertex at line " + std::to_string(lineNumber) + "."
					);
				}
			}
			else if (tag == "f") {
				const Result<void> result = parse_face(stream, parsed);
				if (result.is_error()) {
					return Result<geometry::LEM>::fail(
						result.error().code,
						"Invalid OBJ face at line " + std::to_string(lineNumber) + "."
					);
				}
			}
		}

		if (file.bad()) {
			return Result<geometry::LEM>::fail(ErrorCode::IoError, "Failed while reading OBJ file.");
		}

		if (parsed.positions.empty()) {
			return Result<geometry::LEM>::fail(ErrorCode::DegenerateGeometry, "OBJ import found no vertices.");
		}

		if (options.requireFaces && parsed.faces.empty()) {
			return Result<geometry::LEM>::fail(ErrorCode::DegenerateGeometry, "OBJ import found no faces.");
		}

		return build_mesh(parsed, options);
	}

private:
	/**
	 * @brief Parsed OBJ geometry before conversion to LEM handles.
	 */
	struct ParsedObj {
		/**
		 * @brief Vertex positions in OBJ order.
		 */
		std::vector<glm::vec3> positions{};

		/**
		 * @brief Polygonal faces as zero-based vertex indices.
		 */
		std::vector<std::vector<std::size_t>> faces{};
	};

	/**
	 * @brief Removes leading and trailing whitespace from a string.
	 *
	 * @param text String to modify.
	 */
	static void trim(std::string& text)
	{
		auto isNotSpace = [](unsigned char character) {
			return !std::isspace(character);
		};

		text.erase(text.begin(), std::find_if(text.begin(), text.end(), isNotSpace));
		text.erase(std::find_if(text.rbegin(), text.rend(), isNotSpace).base(), text.end());
	}

	/**
	 * @brief Parses an OBJ vertex record.
	 *
	 * @param stream Line stream positioned after the "v" tag.
	 * @param positions Output position array.
	 * @return Success or parse error.
	 */
	[[nodiscard]] static Result<void> parse_vertex(
		std::istringstream& stream,
		std::vector<glm::vec3>& positions)
	{
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };

		if (!(stream >> position.x >> position.y >> position.z)) {
			return Result<void>::fail(ErrorCode::InvalidArgument, "OBJ vertex requires three coordinates.");
		}

		positions.push_back(position);
		return Result<void>::ok();
	}

	/**
	 * @brief Parses an OBJ face record.
	 *
	 * @param stream Line stream positioned after the "f" tag.
	 * @param parsed Parsed OBJ data.
	 * @return Success or parse error.
	 */
	[[nodiscard]] static Result<void> parse_face(std::istringstream& stream, ParsedObj& parsed)
	{
		std::vector<std::size_t> face{};
		std::string token;

		while (stream >> token) {
			const Result<std::size_t> index = parse_face_vertex_index(token, parsed.positions.size());
			if (index.is_error()) {
				return Result<void>::fail(index.error());
			}

			face.push_back(index.value());
		}

		if (face.size() < 3) {
			return Result<void>::fail(ErrorCode::DegenerateGeometry, "OBJ face requires at least three vertices.");
		}

		parsed.faces.push_back(std::move(face));
		return Result<void>::ok();
	}

	/**
	 * @brief Parses the vertex index part of an OBJ face token.
	 *
	 * @param token Face token in v, v/vt, v//vn, or v/vt/vn form.
	 * @param vertexCount Number of positions parsed so far.
	 * @return Zero-based vertex index or parse error.
	 */
	[[nodiscard]] static Result<std::size_t> parse_face_vertex_index(
		const std::string& token,
		std::size_t vertexCount)
	{
		if (token.empty()) {
			return Result<std::size_t>::fail(ErrorCode::InvalidArgument, "OBJ face token is empty.");
		}

		const std::size_t slash = token.find('/');
		const std::string indexText = slash == std::string::npos
			? token
			: token.substr(0, slash);

		if (indexText.empty()) {
			return Result<std::size_t>::fail(ErrorCode::InvalidArgument, "OBJ face token has no vertex index.");
		}

		int rawIndex = 0;

		try {
			std::size_t consumed = 0;
			rawIndex = std::stoi(indexText, &consumed);

			if (consumed != indexText.size()) {
				return Result<std::size_t>::fail(ErrorCode::InvalidArgument, "OBJ vertex index is not numeric.");
			}
		}
		catch (...) {
			return Result<std::size_t>::fail(ErrorCode::InvalidArgument, "OBJ vertex index is invalid.");
		}

		if (rawIndex == 0) {
			return Result<std::size_t>::fail(ErrorCode::OutOfRange, "OBJ indices are one-based and cannot be zero.");
		}

		const int resolved = rawIndex > 0
			? rawIndex - 1
			: static_cast<int>(vertexCount) + rawIndex;

		if (resolved < 0 || static_cast<std::size_t>(resolved) >= vertexCount) {
			return Result<std::size_t>::fail(ErrorCode::OutOfRange, "OBJ vertex index is outside the parsed vertex range.");
		}

		return Result<std::size_t>::ok(static_cast<std::size_t>(resolved));
	}

	/**
	 * @brief Builds a LEM mesh from parsed OBJ data.
	 *
	 * @param parsed Parsed OBJ geometry.
	 * @param options Import behavior options.
	 * @return Imported mesh or conversion error.
	 */
	[[nodiscard]] static Result<geometry::LEM> build_mesh(
		const ParsedObj& parsed,
		const MeshImportOptions& options)
	{
		geometry::LEM mesh{};
		std::vector<geometry::VertexHandle> handles{};
		handles.reserve(parsed.positions.size());

		for (const glm::vec3& position : parsed.positions) {
			if (options.mergeDuplicateVertices) {
				const geometry::VertexHandle existing = find_existing_vertex(mesh, handles, position, options.mergeEpsilon);
				if (existing.is_valid()) {
					handles.push_back(existing);
					continue;
				}
			}

			handles.push_back(mesh.add_vertex(position));
		}

		std::size_t faceCount = 0;

		for (const std::vector<std::size_t>& face : parsed.faces) {
			std::vector<geometry::VertexHandle> faceVertices{};
			faceVertices.reserve(face.size());

			for (std::size_t index : face) {
				if (index >= handles.size() || !mesh.is_valid(handles[index])) {
					faceVertices.clear();
					break;
				}

				faceVertices.push_back(handles[index]);
			}

			if (!is_valid_face_vertex_list(faceVertices)) {
				continue;
			}

			const geometry::FaceHandle faceHandle = mesh.add_face(faceVertices);
			if (mesh.is_valid(faceHandle)) {
				++faceCount;
			}
		}

		if (options.requireFaces && faceCount == 0) {
			return Result<geometry::LEM>::fail(ErrorCode::DegenerateGeometry, "OBJ import could not build any valid face.");
		}

		if (options.rebuildNormals) {
			geometry::NormalBuilder::rebuild_face_normals(mesh);
		}

		return Result<geometry::LEM>::ok(std::move(mesh));
	}

	/**
	 * @brief Finds an existing vertex within merge tolerance.
	 *
	 * @param mesh Mesh containing previously inserted vertices.
	 * @param handles Handles created for each parsed OBJ vertex.
	 * @param position Position to find.
	 * @param epsilon Maximum accepted distance.
	 * @return Existing handle or invalid handle.
	 */
	[[nodiscard]] static geometry::VertexHandle find_existing_vertex(
		const geometry::LEM& mesh,
		const std::vector<geometry::VertexHandle>& handles,
		const glm::vec3& position,
		float epsilon)
	{
		const float epsilonSquared = epsilon * epsilon;

		for (geometry::VertexHandle handle : handles) {
			if (!mesh.is_valid(handle)) {
				continue;
			}

			const glm::vec3 delta = mesh.vertex(handle).position - position;
			if (glm::dot(delta, delta) <= epsilonSquared) {
				return handle;
			}
		}

		return {};
	}

	/**
	 * @brief Checks whether a face has enough unique vertices for LEM insertion.
	 *
	 * @param vertices Ordered vertex handles.
	 * @return True when the face can be inserted.
	 */
	[[nodiscard]] static bool is_valid_face_vertex_list(const std::vector<geometry::VertexHandle>& vertices)
	{
		if (vertices.size() < 3) {
			return false;
		}

		for (std::size_t i = 0; i < vertices.size(); ++i) {
			if (!vertices[i].is_valid()) {
				return false;
			}

			for (std::size_t j = i + 1; j < vertices.size(); ++j) {
				if (vertices[i] == vertices[j]) {
					return false;
				}
			}
		}

		return true;
	}
};

}