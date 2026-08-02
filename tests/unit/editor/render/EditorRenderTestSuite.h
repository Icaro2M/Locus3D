/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file EditorRenderTestSuite.h
 * @brief Declarations for editor render adapter unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_render_mesh_upload_adapter_tests();
[[nodiscard]] TestResult run_mesh_node_render_adapter_tests();
[[nodiscard]] TestResult run_scene_render_adapter_tests();
[[nodiscard]] TestResult run_selection_render_adapter_tests();
[[nodiscard]] TestResult run_topology_overlay_adapter_tests();
[[nodiscard]] TestResult run_topology_overlay_vertex_marker_tests();
[[nodiscard]] TestResult run_topology_overlay_face_surface_tests();

} // namespace locus::tests
