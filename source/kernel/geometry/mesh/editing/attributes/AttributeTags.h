/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstdint>

namespace locus::kernel::geometry {

    class LEM;

    /**
     * @brief Edits internal tool tags on editable mesh elements.
     */
    class AttributeTags {
    public:
        /**
         * @brief Creates a tag editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives tag edits.
         * @param diff Diff that receives element change events.
         */
        AttributeTags(LEM& mesh, LEMDiff& diff);

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
    };

}