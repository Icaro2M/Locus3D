/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/selection/MeshSelection.h"
#include "editor/selection/SelectionGranularity.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstddef>
#include <vector>

namespace locus::editor {

    /**
     * @brief Stable component target captured by an interactive mesh tool.
     *
     * MeshToolTarget stores identifiers only. It does not own or retain pointers
     * to the editor scene, MeshNode, LEM, or current selection state.
     *
     * Capturing the target when an interaction begins prevents later selection
     * changes from silently changing the components affected by an active tool.
     */
    struct MeshToolTarget {
        /**
         * @brief Scene node containing the editable mesh.
         */
        SceneNodeId nodeId{};

        /**
         * @brief Component granularity captured for the tool.
         */
        SelectionGranularity granularity =
            SelectionGranularity::Object;

        /**
         * @brief Selected vertex handles captured at interaction start.
         */
        std::vector<kernel::geometry::VertexHandle> vertices{};

        /**
         * @brief Selected edge handles captured at interaction start.
         */
        std::vector<kernel::geometry::EdgeHandle> edges{};

        /**
         * @brief Selected loop handles captured at interaction start.
         */
        std::vector<kernel::geometry::LoopHandle> loops{};

        /**
         * @brief Selected face handles captured at interaction start.
         */
        std::vector<kernel::geometry::FaceHandle> faces{};

        /**
         * @brief Creates a target from the current mesh selection.
         *
         * Only the component set corresponding to the requested granularity is
         * captured. This keeps the target unambiguous for concrete mesh tools.
         *
         * @param selection Current editor mesh selection.
         * @param targetGranularity Component granularity required by the tool.
         * @return Captured mesh tool target.
         */
        [[nodiscard]]
        static MeshToolTarget capture(
            const MeshSelection& selection,
            SelectionGranularity targetGranularity)
        {
            MeshToolTarget target{};

            target.nodeId = selection.active_mesh();
            target.granularity = targetGranularity;

            switch (targetGranularity) {
            case SelectionGranularity::Vertex:
                target.vertices =
                    selection.vertices().items();
                break;

            case SelectionGranularity::Edge:
                target.edges =
                    selection.edges().items();
                break;

            case SelectionGranularity::Loop:
                target.loops =
                    selection.loops().items();
                break;

            case SelectionGranularity::Face:
                target.faces =
                    selection.faces().items();
                break;

            case SelectionGranularity::Object:
                break;
            }

            return target;
        }

        /**
         * @brief Creates an empty invalid target.
         *
         * @return Invalid mesh tool target.
         */
        [[nodiscard]]
        static MeshToolTarget none()
        {
            return {};
        }

        /**
         * @brief Checks whether the target references an active mesh node.
         *
         * This check does not verify that the node still exists or remains a
         * MeshNode. Scene-dependent validation belongs to the operation session.
         *
         * @return True when the stored scene node identifier is valid.
         */
        [[nodiscard]]
        bool has_node() const
        {
            return nodeId.is_valid();
        }

        /**
         * @brief Checks whether the target uses mesh-component granularity.
         *
         * @return True for vertex, edge, loop, or face targets.
         */
        [[nodiscard]]
        bool has_component_granularity() const
        {
            return is_mesh_granularity(granularity);
        }

        /**
         * @brief Returns the number of captured target components.
         *
         * Only the component collection corresponding to the target granularity
         * contributes to this count.
         *
         * @return Number of captured components.
         */
        [[nodiscard]]
        std::size_t component_count() const
        {
            switch (granularity) {
            case SelectionGranularity::Vertex:
                return vertices.size();

            case SelectionGranularity::Edge:
                return edges.size();

            case SelectionGranularity::Loop:
                return loops.size();

            case SelectionGranularity::Face:
                return faces.size();

            case SelectionGranularity::Object:
                return 0;
            }

            return 0;
        }

        /**
         * @brief Checks whether the target has no captured components.
         *
         * @return True when component_count() is zero.
         */
        [[nodiscard]]
        bool empty() const
        {
            return component_count() == 0;
        }

        /**
         * @brief Checks whether the target is structurally usable.
         *
         * This verifies the node identifier, component granularity, component
         * count, and stored handle validity. It does not inspect the current LEM.
         *
         * @return True when the target is structurally valid.
         */
        [[nodiscard]]
        bool is_valid() const
        {
            if (!has_node() ||
                !has_component_granularity() ||
                empty()) {
                return false;
            }

            switch (granularity) {
            case SelectionGranularity::Vertex:
                return all_handles_valid(vertices);

            case SelectionGranularity::Edge:
                return all_handles_valid(edges);

            case SelectionGranularity::Loop:
                return all_handles_valid(loops);

            case SelectionGranularity::Face:
                return all_handles_valid(faces);

            case SelectionGranularity::Object:
                return false;
            }

            return false;
        }

        /**
         * @brief Checks whether the target contains vertices.
         *
         * @return True when this is a non-empty vertex target.
         */
        [[nodiscard]]
        bool targets_vertices() const
        {
            return granularity == SelectionGranularity::Vertex &&
                !vertices.empty();
        }

        /**
         * @brief Checks whether the target contains edges.
         *
         * @return True when this is a non-empty edge target.
         */
        [[nodiscard]]
        bool targets_edges() const
        {
            return granularity == SelectionGranularity::Edge &&
                !edges.empty();
        }

        /**
         * @brief Checks whether the target contains loops.
         *
         * @return True when this is a non-empty loop target.
         */
        [[nodiscard]]
        bool targets_loops() const
        {
            return granularity == SelectionGranularity::Loop &&
                !loops.empty();
        }

        /**
         * @brief Checks whether the target contains faces.
         *
         * @return True when this is a non-empty face target.
         */
        [[nodiscard]]
        bool targets_faces() const
        {
            return granularity == SelectionGranularity::Face &&
                !faces.empty();
        }

        /**
         * @brief Removes all target information.
         */
        void clear()
        {
            nodeId = {};
            granularity = SelectionGranularity::Object;

            vertices.clear();
            edges.clear();
            loops.clear();
            faces.clear();
        }

    private:
        /**
         * @brief Checks whether every handle in a collection is valid.
         *
         * @tparam HandleT Kernel handle type.
         * @param handles Handles to inspect.
         * @return True when the collection is non-empty and all handles are valid.
         */
        template<typename HandleT>
        [[nodiscard]]
        static bool all_handles_valid(
            const std::vector<HandleT>& handles)
        {
            if (handles.empty()) {
                return false;
            }

            for (const HandleT handle : handles) {
                if (handle.is_invalid()) {
                    return false;
                }
            }

            return true;
        }
    };

} // namespace locus::editor