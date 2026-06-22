/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/attributes/AttributeTags.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

namespace locus::kernel::geometry {

    AttributeTags::AttributeTags(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    bool AttributeTags::set_tag(VertexHandle handle, std::uint32_t tag)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);
        if (vertex.tag == tag) {
            return true;
        }

        vertex.tag = tag;
        diff_.record(LEMChangeType::VertexModified, handle);
        return true;
    }

    bool AttributeTags::set_tag(EdgeHandle handle, std::uint32_t tag)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Edge& edge = mesh_.edge(handle);
        if (edge.tag == tag) {
            return true;
        }

        edge.tag = tag;
        diff_.record(LEMChangeType::EdgeModified, handle);
        return true;
    }

    bool AttributeTags::set_tag(FaceHandle handle, std::uint32_t tag)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Face& face = mesh_.face(handle);
        if (face.tag == tag) {
            return true;
        }

        face.tag = tag;
        diff_.record(LEMChangeType::FaceModified, handle);
        return true;
    }

    void AttributeTags::clear_tags()
    {
        for (VertexHandle handle : TopologyTraversal::vertices(mesh_)) {
            set_tag(handle, 0);
        }

        for (EdgeHandle handle : TopologyTraversal::edges(mesh_)) {
            set_tag(handle, 0);
        }

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            set_tag(handle, 0);
        }
    }

}