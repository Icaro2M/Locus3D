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
     * @brief Edits visibility flags on editable mesh elements.
     */
    class AttributeVisibility {
    public:
        /**
         * @brief Creates a visibility editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives visibility edits.
         * @param diff Diff that receives visibility change events.
         */
        AttributeVisibility(LEM& mesh, LEMDiff& diff);

        /**
         * @brief Changes a vertex hidden flag.
         *
         * @param handle Vertex to modify.
         * @param hidden New hidden state.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_hidden(VertexHandle handle, bool hidden);

        /**
         * @brief Changes an edge hidden flag.
         *
         * @param handle Edge to modify.
         * @param hidden New hidden state.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_hidden(EdgeHandle handle, bool hidden);

        /**
         * @brief Changes a face hidden flag.
         *
         * @param handle Face to modify.
         * @param hidden New hidden state.
         * @return True when the face exists and the edit was accepted.
         */
        bool set_hidden(FaceHandle handle, bool hidden);

        /**
         * @brief Clears hidden flags on all active vertices, edges, and faces.
         */
        void clear_visibility();

    private:
        LEM& mesh_;
        LEMDiff& diff_;
    };

}