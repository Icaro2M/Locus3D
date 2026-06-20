/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/primitives/BoxBuilder.h"
#include "kernel/geometry/primitives/ConeBuilder.h"
#include "kernel/geometry/primitives/CylinderBuilder.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"
#include "kernel/geometry/primitives/SphereBuilder.h"
#include "kernel/geometry/primitives/TorusBuilder.h"

#include <array>
#include <string_view>
#include <type_traits>
#include <variant>

namespace locus::kernel::geometry {

    /**
     * @brief Registry of built-in primitive builders and default parameters.
     */
    class PrimitiveRegistry {
    public:
        /**
         * @brief Returns all supported built-in primitive types.
         *
         * @return Primitive type list in registry order.
         */
        [[nodiscard]] static constexpr std::array<PrimitiveType, 5> types() {
            return {
                PrimitiveType::Box,
                PrimitiveType::Cylinder,
                PrimitiveType::Sphere,
                PrimitiveType::Cone,
                PrimitiveType::Torus
            };
        }

        /**
         * @brief Returns the display name for a primitive type.
         *
         * @param type Primitive type.
         * @return Stable primitive display name.
         */
        [[nodiscard]] static std::string_view name(PrimitiveType type) {
            switch (type) {
            case PrimitiveType::Box:
                return "Box";
            case PrimitiveType::Cylinder:
                return "Cylinder";
            case PrimitiveType::Sphere:
                return "Sphere";
            case PrimitiveType::Cone:
                return "Cone";
            case PrimitiveType::Torus:
                return "Torus";
            }

            return {};
        }

        /**
         * @brief Returns default parameters for a primitive type.
         *
         * @param type Primitive type.
         * @return Variant containing the matching default parameter struct.
         */
        [[nodiscard]] static PrimitiveParameters default_parameters(PrimitiveType type) {
            switch (type) {
            case PrimitiveType::Box:
                return BoxParameters{};
            case PrimitiveType::Cylinder:
                return CylinderParameters{};
            case PrimitiveType::Sphere:
                return SphereParameters{};
            case PrimitiveType::Cone:
                return ConeParameters{};
            case PrimitiveType::Torus:
                return TorusParameters{};
            }

            return BoxParameters{};
        }

        /**
         * @brief Creates a new mesh containing a primitive.
         *
         * @param parameters Variant containing the primitive parameters.
         * @return Mesh containing the primitive, or an empty mesh on failure.
         */
        [[nodiscard]] static LEM build(const PrimitiveParameters& parameters) {
            LEM mesh;
            build_into(mesh, parameters);
            return mesh;
        }

        /**
         * @brief Appends a primitive to an existing mesh.
         *
         * @param mesh Mesh that receives the primitive topology.
         * @param parameters Variant containing the primitive parameters.
         * @return Created element handles, recorded diff, and success state.
         */
        [[nodiscard]] static PrimitiveBuildResult build_into(
            LEM& mesh,
            const PrimitiveParameters& parameters) {
            return std::visit(
                [&mesh](const auto& typedParameters) -> PrimitiveBuildResult {
                    using Parameters = std::decay_t<decltype(typedParameters)>;

                    if constexpr (std::is_same_v<Parameters, BoxParameters>) {
                        return BoxBuilder::build_into(mesh, typedParameters);
                    }
                    else if constexpr (std::is_same_v<Parameters, CylinderParameters>) {
                        return CylinderBuilder::build_into(mesh, typedParameters);
                    }
                    else if constexpr (std::is_same_v<Parameters, SphereParameters>) {
                        return SphereBuilder::build_into(mesh, typedParameters);
                    }
                    else if constexpr (std::is_same_v<Parameters, ConeParameters>) {
                        return ConeBuilder::build_into(mesh, typedParameters);
                    }
                    else if constexpr (std::is_same_v<Parameters, TorusParameters>) {
                        return TorusBuilder::build_into(mesh, typedParameters);
                    }
                    else {
                        return {};
                    }
                },
                parameters
            );
        }
    };

}
