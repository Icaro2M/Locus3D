/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

namespace locus::kernel::geometry {

    class LEM;

    namespace editing::topology {

        /**
         * @brief Removes a loop from the radial cycle of its current edge.
         *
         * @param mesh Mesh that owns the loop.
         * @param diff Diff recorder that receives modified element events.
         * @param loopHandle Loop to detach from its edge radial cycle.
         * @return True when the loop was valid and could be detached.
         */
        bool remove_loop_from_radial(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle);

        /**
         * @brief Inserts a loop into the radial cycle of an edge.
         *
         * @param mesh Mesh that owns the loop and edge.
         * @param diff Diff recorder that receives modified element events.
         * @param loopHandle Loop to insert.
         * @param edgeHandle Edge that will own the radial cycle.
         * @return True when insertion succeeded.
         */
        bool insert_loop_into_radial(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle, EdgeHandle edgeHandle);

        /**
         * @brief Refreshes the entry loop stored by an edge.
         *
         * @param mesh Mesh that owns the edge.
         * @param diff Diff recorder that receives modified element events.
         * @param edgeHandle Edge whose entry loop may need updating.
         */
        void refresh_edge_entry_loop(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle);

        /**
         * @brief Refreshes the incident edge stored by a vertex.
         *
         * @param mesh Mesh that owns the vertex.
         * @param diff Diff recorder that receives modified element events.
         * @param vertexHandle Vertex whose incident edge may need updating.
         */
        void refresh_vertex_incident_edge(LEM& mesh, LEMDiff& diff, VertexHandle vertexHandle);

        /**
         * @brief Reassigns the vertex referenced by a loop.
         *
         * @param mesh Mesh that owns the loop.
         * @param diff Diff recorder that receives modified element events.
         * @param loopHandle Loop to update.
         * @param vertexHandle New vertex referenced by the loop.
         * @return True when both handles were valid and the loop was updated.
         */
        bool update_loop_vertex(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle, VertexHandle vertexHandle);

        /**
         * @brief Reassigns the edge referenced by a loop and updates radial cycles.
         *
         * @param mesh Mesh that owns the loop and edge.
         * @param diff Diff recorder that receives modified element events.
         * @param loopHandle Loop to update.
         * @param edgeHandle New edge referenced by the loop.
         * @return True when both handles were valid and the loop was relinked.
         */
        bool update_loop_edge(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle, EdgeHandle edgeHandle);

        /**
         * @brief Replaces all occurrences of one vertex in a face boundary.
         *
         * @param mesh Mesh that owns the face.
         * @param diff Diff recorder that receives modified element events.
         * @param faceHandle Face whose boundary loops will be edited.
         * @param oldVertex Vertex to replace.
         * @param newVertex Replacement vertex.
         * @return True when at least one loop was updated.
         */
        bool replace_vertex_in_face(
            LEM& mesh,
            LEMDiff& diff,
            FaceHandle faceHandle,
            VertexHandle oldVertex,
            VertexHandle newVertex);

    }

}