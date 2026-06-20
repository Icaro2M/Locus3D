/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <variant>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Built-in primitive categories supported by the geometry kernel.
     */
    enum class PrimitiveType {
        /**
         * @brief Axis-aligned box primitive.
         */
        Box,
        /**
         * @brief Cylinder primitive aligned to the local Z axis.
         */
        Cylinder,
        /**
         * @brief UV sphere primitive.
         */
        Sphere,
        /**
         * @brief Cone primitive aligned to the local Z axis.
         */
        Cone,
        /**
         * @brief Torus primitive centered around the local Z axis.
         */
        Torus
    };

    /**
     * @brief Mesh elements and diff produced by a primitive builder.
     */
    struct PrimitiveBuildResult {
        /**
         * @brief Vertices created by the primitive build.
         */
        std::vector<VertexHandle> vertices{};
        /**
         * @brief Edges created by the primitive build.
         */
        std::vector<EdgeHandle> edges{};
        /**
         * @brief Faces created by the primitive build.
         */
        std::vector<FaceHandle> faces{};
        /**
         * @brief Mesh changes recorded during the build.
         */
        LEMDiff diff{};
        /**
         * @brief True when the primitive was fully created.
         */
        bool success = false;

        /**
         * @brief Converts the result to true when the build succeeded.
         */
        [[nodiscard]] explicit operator bool() const {
            return success;
        }

        /**
         * @brief Checks whether the build produced no mesh elements.
         *
         * @return True when all created element lists are empty.
         */
        [[nodiscard]] bool empty() const {
            return vertices.empty() && edges.empty() && faces.empty();
        }
    };

    /**
     * @brief Parameters used to create an axis-aligned box primitive.
     */
    struct BoxParameters {
        /**
         * @brief Box center in object space.
         */
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        /**
         * @brief Box dimensions along the local X, Y, and Z axes.
         */
        glm::vec3 size{ 1.0f, 1.0f, 1.0f };
        /**
         * @brief True when faces created by the builder should be selected.
         */
        bool selectCreatedFaces = false;

        /**
         * @brief Checks whether the box dimensions are usable.
         *
         * @return True when every size component is positive.
         */
        [[nodiscard]] bool is_valid() const {
            return size.x > 0.0f && size.y > 0.0f && size.z > 0.0f;
        }
    };

    /**
     * @brief Parameters used to create a cylinder primitive.
     */
    struct CylinderParameters {
        /**
         * @brief Cylinder center in object space.
         */
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        /**
         * @brief Radius of the circular top and bottom rings.
         */
        float radius = 0.5f;
        /**
         * @brief Cylinder height along the local Z axis.
         */
        float height = 1.0f;
        /**
         * @brief Number of radial segments around the cylinder.
         */
        std::size_t segments = 32;
        /**
         * @brief True when a top cap face should be created.
         */
        bool capTop = true;
        /**
         * @brief True when a bottom cap face should be created.
         */
        bool capBottom = true;
        /**
         * @brief True when faces created by the builder should be selected.
         */
        bool selectCreatedFaces = false;

        /**
         * @brief Checks whether the cylinder parameters are usable.
         *
         * @return True when dimensions are positive and at least three segments exist.
         */
        [[nodiscard]] bool is_valid() const {
            return radius > 0.0f && height > 0.0f && segments >= 3;
        }
    };

    /**
     * @brief Parameters used to create a UV sphere primitive.
     */
    struct SphereParameters {
        /**
         * @brief Sphere center in object space.
         */
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        /**
         * @brief Sphere radius.
         */
        float radius = 0.5f;
        /**
         * @brief Number of segments around the equator.
         */
        std::size_t longitudeSegments = 32;
        /**
         * @brief Number of latitude bands between poles.
         */
        std::size_t latitudeSegments = 16;
        /**
         * @brief True when faces created by the builder should be selected.
         */
        bool selectCreatedFaces = false;

        /**
         * @brief Checks whether the sphere parameters are usable.
         *
         * @return True when radius and segment counts can form a sphere.
         */
        [[nodiscard]] bool is_valid() const {
            return radius > 0.0f && longitudeSegments >= 3 && latitudeSegments >= 2;
        }
    };

    /**
     * @brief Parameters used to create a cone primitive.
     */
    struct ConeParameters {
        /**
         * @brief Cone center in object space.
         */
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        /**
         * @brief Radius of the cone base.
         */
        float radius = 0.5f;
        /**
         * @brief Cone height along the local Z axis.
         */
        float height = 1.0f;
        /**
         * @brief Number of radial segments around the cone base.
         */
        std::size_t segments = 32;
        /**
         * @brief True when a bottom cap face should be created.
         */
        bool capBottom = true;
        /**
         * @brief True when faces created by the builder should be selected.
         */
        bool selectCreatedFaces = false;

        /**
         * @brief Checks whether the cone parameters are usable.
         *
         * @return True when dimensions are positive and at least three segments exist.
         */
        [[nodiscard]] bool is_valid() const {
            return radius > 0.0f && height > 0.0f && segments >= 3;
        }
    };

    /**
     * @brief Parameters used to create a torus primitive.
     */
    struct TorusParameters {
        /**
         * @brief Torus center in object space.
         */
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        /**
         * @brief Distance from torus center to the tube centerline.
         */
        float majorRadius = 1.0f;
        /**
         * @brief Radius of the torus tube.
         */
        float minorRadius = 0.25f;
        /**
         * @brief Number of segments around the major ring.
         */
        std::size_t majorSegments = 32;
        /**
         * @brief Number of segments around the tube.
         */
        std::size_t minorSegments = 12;
        /**
         * @brief True when faces created by the builder should be selected.
         */
        bool selectCreatedFaces = false;

        /**
         * @brief Checks whether the torus parameters are usable.
         *
         * @return True when radii and segment counts can form a torus.
         */
        [[nodiscard]] bool is_valid() const {
            return majorRadius > 0.0f
                && minorRadius > 0.0f
                && minorRadius < majorRadius
                && majorSegments >= 3
                && minorSegments >= 3;
        }
    };

    /**
     * @brief Variant containing parameters for any built-in primitive.
     */
    using PrimitiveParameters = std::variant<
        BoxParameters,
        CylinderParameters,
        SphereParameters,
        ConeParameters,
        TorusParameters
    >;

}
