/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::geometry {

    class LEM;

}

namespace locus::kernel::validation {

    /**
     * @brief Input data shared by validation checks.
     */
    struct ValidationContext {
        /**
         * @brief Editable mesh being validated.
         */
        const geometry::LEM* mesh = nullptr;

        /**
         * @brief Checks whether the context has an editable mesh.
         *
         * @return True when mesh points to a valid mesh object.
         */
        [[nodiscard]] bool has_mesh() const
        {
            return mesh != nullptr;
        }
    };

}