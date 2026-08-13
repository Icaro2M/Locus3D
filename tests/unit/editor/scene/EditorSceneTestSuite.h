/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file EditorSceneTestSuite.h
 * @brief Declarations for editor scene unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_node_transform_tests();
[[nodiscard]] TestResult run_scene_node_tests();
[[nodiscard]] TestResult run_scene_tree_tests();
[[nodiscard]] TestResult run_editor_scene_tests();
[[nodiscard]] TestResult run_mesh_node_tests();
[[nodiscard]] TestResult run_document_archive_tests();

} // namespace locus::tests
