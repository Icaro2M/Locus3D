/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/io/IExporter.h"

#include <filesystem>
#include <string_view>
#include <vector>

namespace locus::kernel::io {

/**
 * @brief Describes how mesh geometry should be imported.
 */
struct MeshImportOptions {
	/**
	 * @brief True when importer should attempt to merge duplicated vertices.
	 */
	bool mergeDuplicateVertices = false;

	/**
	 * @brief True when importer should attempt to rebuild missing normals.
	 */
	bool rebuildNormals = true;

	/**
	 * @brief True when importer should reject files that cannot produce any face.
	 */
	bool requireFaces = true;

	/**
	 * @brief Maximum distance used by optional vertex merge operations.
	 */
	float mergeEpsilon = 1.0e-5f;
};

/**
 * @brief Base interface for mesh importers.
 */
class IImporter {
public:
	/**
	 * @brief Destroys the importer interface.
	 */
	virtual ~IImporter() = default;

	/**
	 * @brief Returns the format imported by this object.
	 *
	 * @return Mesh format identifier.
	 */
	[[nodiscard]] virtual MeshFormat format() const = 0;

	/**
	 * @brief Returns the stable importer display name.
	 *
	 * @return Importer name.
	 */
	[[nodiscard]] virtual std::string_view name() const = 0;

	/**
	 * @brief Returns the file extensions supported by this importer.
	 *
	 * @return Supported extensions without dot.
	 */
	[[nodiscard]] virtual std::vector<std::string_view> extensions() const = 0;

	/**
	 * @brief Checks whether this importer supports an extension.
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
	 * @brief Reads an editable mesh from a file.
	 *
	 * @param path Source file path.
	 * @param options Import behavior options.
	 * @return Imported mesh or IO error.
	 */
	[[nodiscard]] virtual Result<geometry::LEM> import_mesh(
		const std::filesystem::path& path,
		const MeshImportOptions& options = {}) const = 0;
};

}