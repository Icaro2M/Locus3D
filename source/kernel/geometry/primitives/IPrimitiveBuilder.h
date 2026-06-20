/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"

#include <string_view>

namespace locus::kernel::geometry {

    /**
     * @brief Interface for builders that create editable mesh primitives.
     */
    class IPrimitiveBuilder {
    public:
        /**
         * @brief Destroys the primitive builder interface.
         */
        virtual ~IPrimitiveBuilder() = default;

        /**
         * @brief Returns the primitive type produced by this builder.
         *
         * @return Primitive type identifier.
         */
        [[nodiscard]] virtual PrimitiveType type() const = 0;
        /**
         * @brief Returns the display name of the primitive builder.
         *
         * @return Builder name.
         */
        [[nodiscard]] virtual std::string_view name() const = 0;

        /**
         * @brief Appends primitive geometry to an existing mesh.
         *
         * @param mesh Mesh that receives the primitive.
         * @param parameters Variant containing the primitive parameters.
         * @return Created element handles, recorded diff, and success state.
         */
        [[nodiscard]] virtual PrimitiveBuildResult build_into(
            LEM& mesh,
            const PrimitiveParameters& parameters) const = 0;

        /**
         * @brief Creates a new mesh containing the primitive.
         *
         * @param parameters Variant containing the primitive parameters.
         * @return Mesh containing the built primitive, or an empty mesh on failure.
         */
        [[nodiscard]] LEM build(const PrimitiveParameters& parameters) const {
            LEM mesh;
            build_into(mesh, parameters);
            return mesh;
        }
    };

}
