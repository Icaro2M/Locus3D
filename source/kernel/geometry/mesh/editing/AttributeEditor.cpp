/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/AttributeEditor.h"

#include "kernel/geometry/topology/TopologyTraversal.h"

namespace locus::kernel::geometry {

    AttributeEditor::AttributeEditor(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    LEM& AttributeEditor::mesh()
    {
        return mesh_;
    }

    const LEM& AttributeEditor::mesh() const
    {
        return mesh_;
    }

    bool AttributeEditor::set_selected(VertexHandle handle, bool selected)
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

    bool AttributeEditor::set_selected(EdgeHandle handle, bool selected)
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

    bool AttributeEditor::set_selected(FaceHandle handle, bool selected)
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

    void AttributeEditor::clear_selection()
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

    bool AttributeEditor::set_hidden(VertexHandle handle, bool hidden)
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

    bool AttributeEditor::set_hidden(EdgeHandle handle, bool hidden)
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

    bool AttributeEditor::set_hidden(FaceHandle handle, bool hidden)
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

    void AttributeEditor::clear_visibility()
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