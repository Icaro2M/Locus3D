/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/attributes/AttributeSelection.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

namespace locus::kernel::geometry {

    AttributeSelection::AttributeSelection(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    bool AttributeSelection::set_selected(VertexHandle handle, bool selected)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);
        if (vertex.selected == selected) {
            return true;
        }

        vertex.selected = selected;
        diff_.record(LEMChangeType::SelectionChanged, handle);
        return true;
    }

    bool AttributeSelection::set_selected(EdgeHandle handle, bool selected)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Edge& edge = mesh_.edge(handle);
        if (edge.selected == selected) {
            return true;
        }

        edge.selected = selected;
        diff_.record(LEMChangeType::SelectionChanged, handle);
        return true;
    }

    bool AttributeSelection::set_selected(FaceHandle handle, bool selected)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Face& face = mesh_.face(handle);
        if (face.selected == selected) {
            return true;
        }

        face.selected = selected;
        diff_.record(LEMChangeType::SelectionChanged, handle);
        return true;
    }

    void AttributeSelection::clear_selection()
    {
        for (VertexHandle handle : TopologyTraversal::vertices(mesh_)) {
            set_selected(handle, false);
        }

        for (EdgeHandle handle : TopologyTraversal::edges(mesh_)) {
            set_selected(handle, false);
        }

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            set_selected(handle, false);
        }
    }

}