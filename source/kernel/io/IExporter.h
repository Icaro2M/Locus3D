/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace locus::kernel::io {

/**
 * @brief Identifies mesh file formats supported by the kernel IO layer.
 */
enum class MeshFormat {
	/**
	 * @brief Stereolithography mesh format.
	 */
	Stl,

	/**
	 * @brief Wavefront OBJ mesh format.
	 */
	Obj,

	/**
	 * @brief 3MF additive manufacturing format.
	 */
	ThreeMF
};

/**
 * @brief Describes how mesh geometry should be exported.
 */
struct MeshExportOptions {
	/**
	 * @brief True when inactive or deleted mesh elements should be skipped.
	 */
	bool skipInactiveElements = true;

	/**
	 * @brief True when derived normals should be written when supported by the format.
	 */
	bool writeNormals = true;

	/**
	 * @brief True when the exporter may triangulate polygonal faces.
	 */
	bool triangulateFaces = true;

	/**
	 * @brief True when the exported file should use binary encoding when the format supports it.
	 */
	bool preferBinary = true;
};

/**
 * @brief Static description of a mesh file format.
 */
struct MeshFormatDescriptor {
	/**
	 * @brief Mesh format identifier.
	 */
	MeshFormat format = MeshFormat::Stl;

	/**
	 * @brief Stable display name.
	 */
	std::string_view name{};

	/**
	 * @brief Primary file extension without dot.
	 */
	std::string_view primaryExtension{};

	/**
	 * @brief Accepted file extensions without dot.
	 */
	std::vector<std::string_view> extensions{};
};

/**
 * @brief Base interface for mesh exporters.
 */
class IExporter {
public:
	/**
	 * @brief Destroys the exporter interface.
	 */
	virtual ~IExporter() = default;

	/**
	 * @brief Returns the format exported by this object.
	 *
	 * @return Mesh format identifier.
	 */
	[[nodiscard]] virtual MeshFormat format() const = 0;

	/**
	 * @brief Returns the stable exporter display name.
	 *
	 * @return Exporter name.
	 */
	[[nodiscard]] virtual std::string_view name() const = 0;

	/**
	 * @brief Returns the file extensions supported by this exporter.
	 *
	 * @return Supported extensions without dot.
	 */
	[[nodiscard]] virtual std::vector<std::string_view> extensions() const = 0;

	/**
	 * @brief Checks whether this exporter supports an extension.
	 *
	 * @param extension File extension with or without dot.
	 * @return True when the extension is supported.
	 */
	[[nodiscard]] bool supports_extension(std::string_view extension) const
	{
		if (!extension.empty() && extension.front() == '.') {
			extension.remove_prefix(1);
		}

		for (std::string_view supported : extensions()) {
			if (supported == extension) {
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Writes an editable mesh to a file.
	 *
	 * @param mesh Mesh to export.
	 * @param path Destination file path.
	 * @param options Export behavior options.
	 * @return Success or IO error.
	 */
	[[nodiscard]] virtual Result<void> export_mesh(
		const geometry::LEM& mesh,
		const std::filesystem::path& path,
		const MeshExportOptions& options = {}) const = 0;
};

/**
 * @brief Returns the stable display name for a mesh format.
 *
 * @param format Mesh format identifier.
 * @return Format display name.
 */
[[nodiscard]] inline std::string_view mesh_format_name(MeshFormat format)
{
	switch (format) {
	case MeshFormat::Stl:
		return "STL";
	case MeshFormat::Obj:
		return "OBJ";
	case MeshFormat::ThreeMF:
		return "3MF";
	}

	return {};
}

/**
 * @brief Returns the primary extension for a mesh format.
 *
 * @param format Mesh format identifier.
 * @return Primary extension without dot.
 */
[[nodiscard]] inline std::string_view mesh_format_primary_extension(MeshFormat format)
{
	switch (format) {
	case MeshFormat::Stl:
		return "stl";
	case MeshFormat::Obj:
		return "obj";
	case MeshFormat::ThreeMF:
		return "3mf";
	}

	return {};
}

/**
 * @brief Returns all accepted extensions for a mesh format.
 *
 * @param format Mesh format identifier.
 * @return Supported extensions without dot.
 */
[[nodiscard]] inline std::vector<std::string_view> mesh_format_extensions(MeshFormat format)
{
	switch (format) {
	case MeshFormat::Stl:
		return { "stl" };
	case MeshFormat::Obj:
		return { "obj" };
	case MeshFormat::ThreeMF:
		return { "3mf" };
	}

	return {};
}

/**
 * @brief Builds a descriptor for a mesh format.
 *
 * @param format Mesh format identifier.
 * @return Format descriptor.
 */
[[nodiscard]] inline MeshFormatDescriptor mesh_format_descriptor(MeshFormat format)
{
	return MeshFormatDescriptor{
		format,
		mesh_format_name(format),
		mesh_format_primary_extension(format),
		mesh_format_extensions(format)
	};
}

}