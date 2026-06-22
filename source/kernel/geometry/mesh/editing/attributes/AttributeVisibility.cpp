/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/attributes/AttributeVisibility.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

namespace locus::kernel::geometry {

    AttributeVisibility::AttributeVisibility(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    bool AttributeVisibility::set_hidden(VertexHandle handle, bool hidden)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);
        if (vertex.hidden == hidden) {
            return true;
        }

        vertex.hidden = hidden;
        diff_.record(LEMChangeType::VisibilityChanged, handle);
        return true;
    }

    bool AttributeVisibility::set_hidden(EdgeHandle handle, bool hidden)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Edge& edge = mesh_.edge(handle);
        if (edge.hidden == hidden) {
            return true;
        }

        edge.hidden = hidden;
        diff_.record(LEMChangeType::VisibilityChanged, handle);
        return true;
    }

    bool AttributeVisibility::set_hidden(FaceHandle handle, bool hidden)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Face& face = mesh_.face(handle);
        if (face.hidden == hidden) {
            return true;
        }

        face.hidden = hidden;
        diff_.record(LEMChangeType::VisibilityChanged, handle);
        return true;
    }

    void AttributeVisibility::clear_visibility()
    {
        for (VertexHandle handle : TopologyTraversal::vertices(mesh_)) {
            set_hidden(handle, false);
        }

        for (EdgeHandle handle : TopologyTraversal::edges(mesh_)) {
            set_hidden(handle, false);
        }

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            set_hidden(handle, false);
        }
    }

}