/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/io/IExporter.h"
#include "kernel/io/IImporter.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace locus::kernel::io {

/**
 * @brief Registry used to discover mesh importers and exporters by format or extension.
 */
class FormatRegistry {
public:
	/**
	 * @brief Registers an exporter.
	 *
	 * @param exporter Exporter instance.
	 * @return True when the exporter was stored.
	 */
	bool register_exporter(std::shared_ptr<IExporter> exporter)
	{
		if (!exporter) {
			return false;
		}

		const MeshFormat format = exporter->format();

		auto existing = std::find_if(
			exporters_.begin(),
			exporters_.end(),
			[format](const std::shared_ptr<IExporter>& current) {
				return current && current->format() == format;
			}
		);

		if (existing != exporters_.end()) {
			*existing = std::move(exporter);
			return true;
		}

		exporters_.push_back(std::move(exporter));
		return true;
	}

	/**
	 * @brief Registers an importer.
	 *
	 * @param importer Importer instance.
	 * @return True when the importer was stored.
	 */
	bool register_importer(std::shared_ptr<IImporter> importer)
	{
		if (!importer) {
			return false;
		}

		const MeshFormat format = importer->format();

		auto existing = std::find_if(
			importers_.begin(),
			importers_.end(),
			[format](const std::shared_ptr<IImporter>& current) {
				return current && current->format() == format;
			}
		);

		if (existing != importers_.end()) {
			*existing = std::move(importer);
			return true;
		}

		importers_.push_back(std::move(importer));
		return true;
	}

	/**
	 * @brief Removes all registered importers and exporters.
	 */
	void clear()
	{
		exporters_.clear();
		importers_.clear();
	}

	/**
	 * @brief Checks whether the registry contains no importers or exporters.
	 *
	 * @return True when both internal lists are empty.
	 */
	[[nodiscard]] bool empty() const
	{
		return exporters_.empty() && importers_.empty();
	}

	/**
	 * @brief Returns the number of registered exporters.
	 *
	 * @return Exporter count.
	 */
	[[nodiscard]] std::size_t exporter_count() const
	{
		return exporters_.size();
	}

	/**
	 * @brief Returns the number of registered importers.
	 *
	 * @return Importer count.
	 */
	[[nodiscard]] std::size_t importer_count() const
	{
		return importers_.size();
	}

	/**
	 * @brief Returns all registered exporters.
	 *
	 * @return Registered exporter list.
	 */
	[[nodiscard]] const std::vector<std::shared_ptr<IExporter>>& exporters() const
	{
		return exporters_;
	}

	/**
	 * @brief Returns all registered importers.
	 *
	 * @return Registered importer list.
	 */
	[[nodiscard]] const std::vector<std::shared_ptr<IImporter>>& importers() const
	{
		return importers_;
	}

	/**
	 * @brief Finds an exporter by mesh format.
	 *
	 * @param format Mesh format identifier.
	 * @return Exporter pointer or nullptr.
	 */
	[[nodiscard]] std::shared_ptr<IExporter> find_exporter(MeshFormat format) const
	{
		auto found = std::find_if(
			exporters_.begin(),
			exporters_.end(),
			[format](const std::shared_ptr<IExporter>& exporter) {
				return exporter && exporter->format() == format;
			}
		);

		if (found == exporters_.end()) {
			return nullptr;
		}

		return *found;
	}

	/**
	 * @brief Finds an importer by mesh format.
	 *
	 * @param format Mesh format identifier.
	 * @return Importer pointer or nullptr.
	 */
	[[nodiscard]] std::shared_ptr<IImporter> find_importer(MeshFormat format) const
	{
		auto found = std::find_if(
			importers_.begin(),
			importers_.end(),
			[format](const std::shared_ptr<IImporter>& importer) {
				return importer && importer->format() == format;
			}
		);

		if (found == importers_.end()) {
			return nullptr;
		}

		return *found;
	}

	/**
	 * @brief Finds an exporter that supports a file extension.
	 *
	 * @param extension File extension with or without dot.
	 * @return Exporter pointer or nullptr.
	 */
	[[nodiscard]] std::shared_ptr<IExporter> find_exporter_by_extension(std::string_view extension) const
	{
		const std::string normalized = normalize_extension(extension);

		auto found = std::find_if(
			exporters_.begin(),
			exporters_.end(),
			[&normalized](const std::shared_ptr<IExporter>& exporter) {
				return exporter && exporter->supports_extension(normalized);
			}
		);

		if (found == exporters_.end()) {
			return nullptr;
		}

		return *found;
	}

	/**
	 * @brief Finds an importer that supports a file extension.
	 *
	 * @param extension File extension with or without dot.
	 * @return Importer pointer or nullptr.
	 */
	[[nodiscard]] std::shared_ptr<IImporter> find_importer_by_extension(std::string_view extension) const
	{
		const std::string normalized = normalize_extension(extension);

		auto found = std::find_if(
			importers_.begin(),
			importers_.end(),
			[&normalized](const std::shared_ptr<IImporter>& importer) {
				return importer && importer->supports_extension(normalized);
			}
		);

		if (found == importers_.end()) {
			return nullptr;
		}

		return *found;
	}

	/**
	 * @brief Finds an exporter that supports the extension of a file path.
	 *
	 * @param path File path.
	 * @return Exporter pointer or nullptr.
	 */
	[[nodiscard]] std::shared_ptr<IExporter> find_exporter_for_path(const std::filesystem::path& path) const
	{
		return find_exporter_by_extension(path.extension().string());
	}

	/**
	 * @brief Finds an importer that supports the extension of a file path.
	 *
	 * @param path File path.
	 * @return Importer pointer or nullptr.
	 */
	[[nodiscard]] std::shared_ptr<IImporter> find_importer_for_path(const std::filesystem::path& path) const
	{
		return find_importer_by_extension(path.extension().string());
	}

	/**
	 * @brief Checks whether an export format is registered.
	 *
	 * @param format Mesh format identifier.
	 * @return True when an exporter is available.
	 */
	[[nodiscard]] bool can_export(MeshFormat format) const
	{
		return find_exporter(format) != nullptr;
	}

	/**
	 * @brief Checks whether an import format is registered.
	 *
	 * @param format Mesh format identifier.
	 * @return True when an importer is available.
	 */
	[[nodiscard]] bool can_import(MeshFormat format) const
	{
		return find_importer(format) != nullptr;
	}

	/**
	 * @brief Checks whether a path has a registered export format.
	 *
	 * @param path File path.
	 * @return True when an exporter is available.
	 */
	[[nodiscard]] bool can_export_path(const std::filesystem::path& path) const
	{
		return find_exporter_for_path(path) != nullptr;
	}

	/**
	 * @brief Checks whether a path has a registered import format.
	 *
	 * @param path File path.
	 * @return True when an importer is available.
	 */
	[[nodiscard]] bool can_import_path(const std::filesystem::path& path) const
	{
		return find_importer_for_path(path) != nullptr;
	}

private:
	/**
	 * @brief Normalizes a file extension for registry lookup.
	 *
	 * @param extension File extension with or without dot.
	 * @return Lowercase extension without dot.
	 */
	[[nodiscard]] static std::string normalize_extension(std::string_view extension)
	{
		if (!extension.empty() && extension.front() == '.') {
			extension.remove_prefix(1);
		}

		std::string normalized{ extension };
		std::transform(
			normalized.begin(),
			normalized.end(),
			normalized.begin(),
			[](unsigned char character) {
				return static_cast<char>(std::tolower(character));
			}
		);

		return normalized;
	}

	std::vector<std::shared_ptr<IExporter>> exporters_{};
	std::vector<std::shared_ptr<IImporter>> importers_{};
};

}