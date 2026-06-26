/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file TestFileUtils.h
 * @brief Simple filesystem helpers for tests.
 */

#include "TestConfig.h"

#include <filesystem>

namespace locus::tests {

/**
 * @brief Checks whether a path exists and is a regular file.
 *
 * @param path Path to inspect.
 * @return True when the path exists and is a regular file.
 */
[[nodiscard]] inline bool file_exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

/**
 * @brief Joins two filesystem path fragments.
 *
 * @param base Base path.
 * @param child Child path.
 * @return Combined path.
 */
[[nodiscard]] inline std::filesystem::path join_path(
    const std::filesystem::path& base,
    const std::filesystem::path& child)
{
    return base / child;
}

/**
 * @brief Returns the relative test data directory.
 *
 * @return Relative path to tests/data.
 */
[[nodiscard]] inline std::filesystem::path test_data_dir()
{
    return std::filesystem::path(TestPaths::data_dir);
}

/**
 * @brief Returns the relative test mesh data directory.
 *
 * @return Relative path to tests/data/meshes.
 */
[[nodiscard]] inline std::filesystem::path test_meshes_dir()
{
    return std::filesystem::path(TestPaths::meshes_dir);
}

/**
 * @brief Returns the relative test scene data directory.
 *
 * @return Relative path to tests/data/scenes.
 */
[[nodiscard]] inline std::filesystem::path test_scenes_dir()
{
    return std::filesystem::path(TestPaths::scenes_dir);
}

/**
 * @brief Returns the relative expected-output data directory.
 *
 * @return Relative path to tests/data/expected.
 */
[[nodiscard]] inline std::filesystem::path test_expected_dir()
{
    return std::filesystem::path(TestPaths::expected_dir);
}

} // namespace locus::tests
