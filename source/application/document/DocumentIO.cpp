/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/document/DocumentIO.h"

#include "editor/io/Locus3DFormat.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace locus::application {

    namespace {

        [[nodiscard]] ApplicationError runtime_error(std::string message)
        {
            return ApplicationError::make(
                ApplicationErrorCode::RuntimeFailure,
                std::move(message));
        }

        [[nodiscard]] std::filesystem::path temp_path_for(
            const std::filesystem::path& path)
        {
            std::filesystem::path temp = path;
            temp += ".tmp";

            std::uint32_t suffix = 0u;
            while (std::filesystem::exists(temp)) {
                temp = path;
                temp += ".tmp";
                temp += std::to_string(++suffix);
            }

            return temp;
        }

    } // namespace

    std::filesystem::path ensure_locus_extension(
        std::filesystem::path path)
    {
        std::string extension = path.extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char value) {
                return static_cast<char>(std::tolower(value));
            });

        if (extension != ".locus") {
            path += ".locus";
        }
        return path;
    }

    ApplicationResult<void> write_locus_document_atomic(
        const std::filesystem::path& path,
        const editor::DocumentArchive& archive)
    {
        const std::string text = editor::serialize_locus_document(archive);
        const std::filesystem::path target = ensure_locus_extension(path);
        const std::filesystem::path parent = target.parent_path();

        std::error_code error{};
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, error);
            if (error) {
                return runtime_error(error.message());
            }
        }

        const std::filesystem::path temp = temp_path_for(target);

        {
            std::ofstream stream(
                temp,
                std::ios::binary | std::ios::trunc);
            if (!stream.is_open()) {
                return runtime_error("Failed to create temporary document file.");
            }

            stream.write(text.data(), static_cast<std::streamsize>(text.size()));
            stream.flush();
            if (!stream.good()) {
                stream.close();
                std::filesystem::remove(temp, error);
                return runtime_error("Failed to write document data.");
            }
        }

        std::filesystem::rename(temp, target, error);
        if (error) {
            std::filesystem::remove(target, error);
            error.clear();
            std::filesystem::rename(temp, target, error);
        }

        if (error) {
            std::filesystem::remove(temp, error);
            return runtime_error("Failed to replace document file atomically.");
        }

        std::filesystem::remove(temp, error);
        return {};
    }

    ApplicationResult<editor::DocumentArchive> read_locus_document(
        const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open()) {
            return runtime_error("Failed to open Locus3D document.");
        }

        std::ostringstream buffer{};
        buffer << stream.rdbuf();
        if (!stream.good() && !stream.eof()) {
            return runtime_error("Failed to read Locus3D document.");
        }

        editor::DocumentArchiveResult result =
            editor::deserialize_locus_document(buffer.str());
        if (!result.success) {
            return runtime_error(result.message);
        }

        return std::move(result.archive);
    }

} // namespace locus::application
