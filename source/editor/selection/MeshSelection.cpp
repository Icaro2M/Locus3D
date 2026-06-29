/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/selection/MeshSelection.h"

namespace locus::editor {

    void MeshSelection::set_active_mesh(SceneNodeId id)
    {
        if (activeMesh_ == id) {
            return;
        }

        activeMesh_ = id;
        clear_components();
    }

    SceneNodeId MeshSelection::active_mesh() const
    {
        return activeMesh_;
    }

    void MeshSelection::clear()
    {
        activeMesh_ = {};
        clear_components();
    }

    void MeshSelection::clear_components()
    {
        vertices_.clear();
        edges_.clear();
        loops_.clear();
        faces_.clear();
        clear_hovered();
    }

    void MeshSelection::set_vertex(kernel::geometry::VertexHandle handle)
    {
        vertices_.clear();

        if (handle.is_valid()) {
            vertices_.add(handle);
        }
    }

    void MeshSelection::set_edge(kernel::geometry::EdgeHandle handle)
    {
        edges_.clear();

        if (handle.is_valid()) {
            edges_.add(handle);
        }
    }

    void MeshSelection::set_loop(kernel::geometry::LoopHandle handle)
    {
        loops_.clear();

        if (handle.is_valid()) {
            loops_.add(handle);
        }
    }

    void MeshSelection::set_face(kernel::geometry::FaceHandle handle)
    {
        faces_.clear();

        if (handle.is_valid()) {
            faces_.add(handle);
        }
    }

    bool MeshSelection::add_vertex(kernel::geometry::VertexHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return vertices_.add(handle);
    }

    bool MeshSelection::add_edge(kernel::geometry::EdgeHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return edges_.add(handle);
    }

    bool MeshSelection::add_loop(kernel::geometry::LoopHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return loops_.add(handle);
    }

    bool MeshSelection::add_face(kernel::geometry::FaceHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return faces_.add(handle);
    }

    bool MeshSelection::remove_vertex(kernel::geometry::VertexHandle handle)
    {
        return vertices_.remove(handle);
    }

    bool MeshSelection::remove_edge(kernel::geometry::EdgeHandle handle)
    {
        return edges_.remove(handle);
    }

    bool MeshSelection::remove_loop(kernel::geometry::LoopHandle handle)
    {
        return loops_.remove(handle);
    }

    bool MeshSelection::remove_face(kernel::geometry::FaceHandle handle)
    {
        return faces_.remove(handle);
    }

    bool MeshSelection::toggle_vertex(kernel::geometry::VertexHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return vertices_.toggle(handle);
    }

    bool MeshSelection::toggle_edge(kernel::geometry::EdgeHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return edges_.toggle(handle);
    }

    bool MeshSelection::toggle_loop(kernel::geometry::LoopHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return loops_.toggle(handle);
    }

    bool MeshSelection::toggle_face(kernel::geometry::FaceHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        return faces_.toggle(handle);
    }

    const SelectionSet<kernel::geometry::VertexHandle>& MeshSelection::vertices() const
    {
        return vertices_;
    }

    const SelectionSet<kernel::geometry::EdgeHandle>& MeshSelection::edges() const
    {
        return edges_;
    }

    const SelectionSet<kernel::geometry::LoopHandle>& MeshSelection::loops() const
    {
        return loops_;
    }

    const SelectionSet<kernel::geometry::FaceHandle>& MeshSelection::faces() const
    {
        return faces_;
    }

    kernel::geometry::VertexHandle MeshSelection::hovered_vertex() const
    {
        return hoveredVertex_;
    }

    kernel::geometry::EdgeHandle MeshSelection::hovered_edge() const
    {
        return hoveredEdge_;
    }

    kernel::geometry::LoopHandle MeshSelection::hovered_loop() const
    {
        return hoveredLoop_;
    }

    kernel::geometry::FaceHandle MeshSelection::hovered_face() const
    {
        return hoveredFace_;
    }

    void MeshSelection::set_hovered_vertex(kernel::geometry::VertexHandle handle)
    {
        hoveredVertex_ = handle;
    }

    void MeshSelection::set_hovered_edge(kernel::geometry::EdgeHandle handle)
    {
        hoveredEdge_ = handle;
    }

    void MeshSelection::set_hovered_loop(kernel::geometry::LoopHandle handle)
    {
        hoveredLoop_ = handle;
    }

    void MeshSelection::set_hovered_face(kernel::geometry::FaceHandle handle)
    {
        hoveredFace_ = handle;
    }

    void MeshSelection::clear_hovered()
    {
        hoveredVertex_ = {};
        hoveredEdge_ = {};
        hoveredLoop_ = {};
        hoveredFace_ = {};
    }

    bool MeshSelection::empty() const
    {
        return vertices_.empty() &&
            edges_.empty() &&
            loops_.empty() &&
            faces_.empty();
    }

}