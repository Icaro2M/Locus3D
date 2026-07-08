/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Logical handle selected on a transform gizmo.
     *
     * Axis handles constrain to one axis, plane handles constrain to two axes,
     * XYZ represents free/uniform manipulation, and View represents a
     * camera-facing/screen-space handle.
     */
    enum class GizmoAxis {
        /**
         * @brief No handle.
         */
        None,

        /**
         * @brief X axis handle.
         */
        X,

        /**
         * @brief Y axis handle.
         */
        Y,

        /**
         * @brief Z axis handle.
         */
        Z,

        /**
         * @brief XY plane handle.
         */
        XY,

        /**
         * @brief XZ plane handle.
         */
        XZ,

        /**
         * @brief YZ plane handle.
         */
        YZ,

        /**
         * @brief Free three-axis handle or uniform handle.
         */
        XYZ,

        /**
         * @brief View-aligned handle.
         */
        View
    };

    /**
     * @brief Checks whether a gizmo handle represents a single axis.
     *
     * @param axis Handle to inspect.
     * @return True when the handle is X, Y, or Z.
     */
    [[nodiscard]] constexpr bool is_gizmo_single_axis(GizmoAxis axis)
    {
        return axis == GizmoAxis::X || axis == GizmoAxis::Y || axis == GizmoAxis::Z;
    }

    /**
     * @brief Checks whether a gizmo handle represents a transform plane.
     *
     * @param axis Handle to inspect.
     * @return True when the handle is XY, XZ, or YZ.
     */
    [[nodiscard]] constexpr bool is_gizmo_plane_axis(GizmoAxis axis)
    {
        return axis == GizmoAxis::XY || axis == GizmoAxis::XZ || axis == GizmoAxis::YZ;
    }

    /**
     * @brief Checks whether a gizmo handle represents free manipulation.
     *
     * @param axis Handle to inspect.
     * @return True when the handle is XYZ.
     */
    [[nodiscard]] constexpr bool is_gizmo_free_axis(GizmoAxis axis)
    {
        return axis == GizmoAxis::XYZ;
    }

} // namespace locus::editor