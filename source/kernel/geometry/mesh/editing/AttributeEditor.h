/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEM.h"

namespace locus::kernel::geometry {

    /**
     * @brief Low-level editor for LEM element attributes.
     *
     * AttributeEditor changes non-topological and non-geometric element state,
     * such as selection and visibility flags. It records accepted changes into
     * the shared LEMDiff owned by the parent LEMEditor facade.
     */
    class AttributeEditor {
    public:
        /**
         * @brief Creates an attribute editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives attribute mutations.
         * @param diff Diff that receives change events.
         */
        AttributeEditor(LEM& mesh, LEMDiff& diff);

        /**
         * @brief Returns the edited mesh.
         *
         * @return Mutable mesh reference.
         */
        [[nodiscard]] LEM& mesh();

        /**
         * @brief Returns the edited mesh.
         *
         * @return Read-only mesh reference.
         */
        [[nodiscard]] const LEM& mesh() const;

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

        /**
         * @brief Changes a vertex visibility flag.
         *
         * @param handle Vertex to modify.
         * @param hidden New hidden state.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_hidden(VertexHandle handle, bool hidden);

        /**
         * @brief Changes an edge visibility flag.
         *
         * @param handle Edge to modify.
         * @param hidden New hidden state.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_hidden(EdgeHandle handle, bool hidden);

        /**
         * @brief Changes a face visibility flag.
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