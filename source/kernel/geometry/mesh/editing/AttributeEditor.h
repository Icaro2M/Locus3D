/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/editing/attributes/AttributeSelection.h"
#include "kernel/geometry/mesh/editing/attributes/AttributeShading.h"
#include "kernel/geometry/mesh/editing/attributes/AttributeTags.h"
#include "kernel/geometry/mesh/editing/attributes/AttributeVisibility.h"

#include <cstdint>

namespace locus::kernel::geometry {

    class LEM;

    /**
     * @brief Facade for non-topological element attributes on a Locus Editable Mesh.
     *
     * AttributeEditor exposes a stable public access point for selection,
     * visibility, shading, and internal tag operations. The implementation is
     * delegated to smaller internal attribute modules under editing/attributes.
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

        /**
         * @brief Changes a vertex internal tag.
         *
         * @param handle Vertex to modify.
         * @param tag New tag value.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_tag(VertexHandle handle, std::uint32_t tag);

        /**
         * @brief Changes an edge internal tag.
         *
         * @param handle Edge to modify.
         * @param tag New tag value.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_tag(EdgeHandle handle, std::uint32_t tag);

        /**
         * @brief Changes a face internal tag.
         *
         * @param handle Face to modify.
         * @param tag New tag value.
         * @return True when the face exists and the edit was accepted.
         */
        bool set_tag(FaceHandle handle, std::uint32_t tag);

        /**
         * @brief Clears internal tags on all active vertices, edges, and faces.
         */
        void clear_tags();

    private:
        LEM& mesh_;
        LEMDiff& diff_;
        AttributeSelection selection_;
        AttributeVisibility visibility_;
        AttributeShading shading_;
        AttributeTags tags_;
    };

}