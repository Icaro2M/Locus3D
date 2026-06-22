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
     * @brief Edits selection flags on editable mesh elements.
     */
    class AttributeSelection {
    public:
        /**
         * @brief Creates a selection editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives selection edits.
         * @param diff Diff that receives selection change events.
         */
        AttributeSelection(LEM& mesh, LEMDiff& diff);

        /**
         * @brief Changes a vertex selection flag.
         *
         * @param handle Vertex to modify.
         * @param selected New selection state.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_selected(VertexHandle handle, bool selected);

        /**
         * @brief Changes an edge selection flag.
         *
         * @param handle Edge to modify.
         * @param selected New selection state.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_selected(EdgeHandle handle, bool selected);

        /**
         * @brief Changes a face selection flag.
         *
         * @param handle Face to modify.
         * @param selected New selection state.
         * @return True when the face exists and the edit was accepted.
         */
        bool set_selected(FaceHandle handle, bool selected);

        /**
         * @brief Clears selection flags on all active vertices, edges, and faces.
         */
        void clear_selection();

    private:
        LEM& mesh_;
        LEMDiff& diff_;
    };

}