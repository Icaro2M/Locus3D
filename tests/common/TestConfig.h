/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file TestConfig.h
 * @brief Shared lightweight configuration values for Locus3D tests.
 */

#include <string_view>

namespace locus::tests {

/**
 * @brief Describes how much text a test helper should print.
 */
enum class TestOutputMode {
    quiet,
    normal,
    verbose
};

/**
 * @brief Common relative paths used by tests.
 */
struct TestPaths {
    static constexpr std::string_view data_dir = "tests/data";
    static constexpr std::string_view meshes_dir = "tests/data/meshes";
    static constexpr std::string_view scenes_dir = "tests/data/scenes";
    static constexpr std::string_view expected_dir = "tests/data/expected";
};

} // namespace locus::tests
