/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file TestMeshFactory.h
 * @brief Future factory helpers for mesh objects used by tests.
 */

namespace locus::tests {

/**
 * @brief Placeholder for future mesh factory helpers.
 *
 * This type intentionally does not include kernel mesh headers yet. The kernel
 * mesh API can evolve independently while the test infrastructure is prepared.
 *
 * Planned helpers include:
 * - make_quad_mesh()
 * - make_cube_mesh()
 * - make_ngon_mesh()
 * - make_nonmanifold_sample()
 */
struct TestMeshFactory {
};

} // namespace locus::tests
