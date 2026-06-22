/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

namespace locus::kernel::geometry {

    class LEM;

    /**
     * @brief Edits shading-related attributes on editable mesh elements.
     */
    class AttributeShading {
    public:
        /**
         * @brief Creates a shading editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives shading edits.
         * @param diff Diff that receives edge change events.
         */
        AttributeShading(LEM& mesh, LEMDiff& diff);

        /**
         * @brief Changes whether adjacent faces should be smoothed across an edge.
         *
         * @param handle Edge to modify.
         * @param smooth New smoothing state.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_smooth(EdgeHandle handle, bool smooth);

        /**
         * @brief Changes crease strength on an edge.
         *
         * @param handle Edge to modify.
         * @param crease New crease strength clamped to the range [0, 1].
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_crease(EdgeHandle handle, float crease);

    private:
        LEM& mesh_;
        LEMDiff& diff_;
    };

}