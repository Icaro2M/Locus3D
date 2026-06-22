/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/AttributeEditor.h"

namespace locus::kernel::geometry {

    AttributeEditor::AttributeEditor(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
        , selection_(mesh, diff)
        , visibility_(mesh, diff)
        , shading_(mesh, diff)
        , tags_(mesh, diff)
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
        return selection_.set_selected(handle, selected);
    }

    bool AttributeEditor::set_selected(EdgeHandle handle, bool selected)
    {
        return selection_.set_selected(handle, selected);
    }

    bool AttributeEditor::set_selected(FaceHandle handle, bool selected)
    {
        return selection_.set_selected(handle, selected);
    }

    void AttributeEditor::clear_selection()
    {
        selection_.clear_selection();
    }

    bool AttributeEditor::set_hidden(VertexHandle handle, bool hidden)
    {
        return visibility_.set_hidden(handle, hidden);
    }

    bool AttributeEditor::set_hidden(EdgeHandle handle, bool hidden)
    {
        return visibility_.set_hidden(handle, hidden);
    }

    bool AttributeEditor::set_hidden(FaceHandle handle, bool hidden)
    {
        return visibility_.set_hidden(handle, hidden);
    }

    void AttributeEditor::clear_visibility()
    {
        visibility_.clear_visibility();
    }

    bool AttributeEditor::set_smooth(EdgeHandle handle, bool smooth)
    {
        return shading_.set_smooth(handle, smooth);
    }

    bool AttributeEditor::set_crease(EdgeHandle handle, float crease)
    {
        return shading_.set_crease(handle, crease);
    }

    bool AttributeEditor::set_tag(VertexHandle handle, std::uint32_t tag)
    {
        return tags_.set_tag(handle, tag);
    }

    bool AttributeEditor::set_tag(EdgeHandle handle, std::uint32_t tag)
    {
        return tags_.set_tag(handle, tag);
    }

    bool AttributeEditor::set_tag(FaceHandle handle, std::uint32_t tag)
    {
        return tags_.set_tag(handle, tag);
    }

    void AttributeEditor::clear_tags()
    {
        tags_.clear_tags();
    }

}