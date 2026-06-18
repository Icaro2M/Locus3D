/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"

namespace locus::kernel::modeling {

/**
 * @brief Mutable execution context shared by modeling operations.
 */
struct OperationContext {
    /**
     * @brief Editable mesh targeted by the operation.
     */
    geometry::LEM* mesh = nullptr;
    /**
     * @brief True when topology validation should run after execution.
     */
    bool validateAfterExecute = true;
    /**
     * @brief True when operations should rebuild affected normals.
     */
    bool rebuildNormals = true;
    /**
     * @brief True when operations may temporarily or permanently create non-manifold topology.
     */
    bool allowNonManifold = true;

    /**
     * @brief Checks whether the context contains a mesh.
     *
     * @return True when mesh points to an editable mesh.
     */
    [[nodiscard]] bool has_mesh() const
    {
        return mesh != nullptr;
    }

    /**
     * @brief Returns the editable mesh.
     *
     * @return Mutable mesh reference.
     * @note Call only after has_mesh() returns true.
     */
    [[nodiscard]] geometry::LEM& editable_mesh()
    {
        return *mesh;
    }

    /**
     * @brief Returns the editable mesh.
     *
     * @return Read-only mesh reference.
     * @note Call only after has_mesh() returns true.
     */
    [[nodiscard]] const geometry::LEM& editable_mesh() const
    {
        return *mesh;
    }
};

}
