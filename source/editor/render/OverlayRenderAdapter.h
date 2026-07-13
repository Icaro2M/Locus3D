/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/primitives/PrimitiveMesh.h"

#include <cstddef>
#include <string>
#include <vector>

namespace locus::editor {

    class MeshNode;
    class MeshSelection;

    /**
     * @brief Semantic role assigned to one generated overlay primitive group.
     *
     * The role remains in the editor layer because it describes the meaning of
     * graphical geometry rather than its low-level primitive topology.
     */
    enum class OverlayPrimitiveRole {
        Wireframe,
        SelectedVertices,
        SelectedEdges,
        SelectedFaces,
        HoveredVertex,
        HoveredEdge,
        HoveredFace
    };

    /**
     * @brief One semantic group of graphical overlay primitives.
     *
     * Each group contains exactly one primitive topology through PrimitiveMesh.
     * Different semantic groups remain separate even when they use the same
     * topology, allowing later render stages to assign independent render state.
     */
    struct OverlayPrimitiveGroup {
        /**
         * @brief Editor meaning of this primitive group.
         */
        OverlayPrimitiveRole role = OverlayPrimitiveRole::Wireframe;

        /**
         * @brief CPU-side primitive geometry generated for the group.
         */
        graphics::PrimitiveMesh mesh{};

        /**
         * @brief Checks whether this group contains drawable geometry.
         *
         * @return True when the primitive mesh is structurally valid.
         */
        [[nodiscard]] bool has_geometry() const {
            return mesh.is_valid();
        }
    };

    /**
     * @brief Complete object-space overlay geometry generated for one mesh node.
     *
     * Geometry remains in the local object space of the source MeshNode. A later
     * render integration stage can reuse the node transform when creating overlay
     * RenderObjects.
     */
    struct OverlayGeometry {
        /**
         * @brief Source editor mesh node.
         */
        SceneNodeId nodeId{};

        /**
         * @brief Independently renderable semantic primitive groups.
         */
        std::vector<OverlayPrimitiveGroup> groups;

        /**
         * @brief Checks whether at least one group contains drawable geometry.
         *
         * @return True when drawable overlay geometry exists.
         */
        [[nodiscard]] bool has_geometry() const {
            for (const OverlayPrimitiveGroup& group : groups) {
                if (group.has_geometry()) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Finds the first group with a semantic role.
         *
         * @param role Role to locate.
         * @return Group pointer, or null when no matching group exists.
         */
        [[nodiscard]] const OverlayPrimitiveGroup* find_group(
            OverlayPrimitiveRole role
        ) const {
            for (const OverlayPrimitiveGroup& group : groups) {
                if (group.role == role) {
                    return &group;
                }
            }

            return nullptr;
        }
    };

    /**
     * @brief Controls which mesh overlays are generated and how they are colored.
     */
    struct OverlayRenderOptions {
        /**
         * @brief Generates the complete visible topological wireframe.
         */
        bool includeWireframe = true;

        /**
         * @brief Generates points for selected mesh vertices.
         */
        bool includeSelectedVertices = true;

        /**
         * @brief Generates lines for selected mesh edges.
         */
        bool includeSelectedEdges = true;

        /**
         * @brief Generates filled triangles for selected mesh faces.
         */
        bool includeSelectedFaces = true;

        /**
         * @brief Generates a point for the hovered vertex.
         */
        bool includeHoveredVertex = true;

        /**
         * @brief Generates a line for the hovered edge.
         */
        bool includeHoveredEdge = true;

        /**
         * @brief Generates filled triangles for the hovered face.
         */
        bool includeHoveredFace = true;

        /**
         * @brief Excludes components marked hidden in the editable mesh.
         */
        bool skipHiddenComponents = true;

        /**
         * @brief Color assigned to the complete topological wireframe.
         */
        graphics::ColorRGBA wireframeColor{
            0.15f,
            0.15f,
            0.15f,
            1.0f
        };

        /**
         * @brief Color assigned to selected vertices.
         */
        graphics::ColorRGBA selectedVertexColor{
            1.0f,
            0.55f,
            0.05f,
            1.0f
        };

        /**
         * @brief Color assigned to selected edges.
         */
        graphics::ColorRGBA selectedEdgeColor{
            1.0f,
            0.55f,
            0.05f,
            1.0f
        };

        /**
         * @brief Color assigned to selected faces.
         */
        graphics::ColorRGBA selectedFaceColor{
            1.0f,
            0.55f,
            0.05f,
            0.35f
        };

        /**
         * @brief Color assigned to hovered vertices.
         */
        graphics::ColorRGBA hoveredVertexColor{
            1.0f,
            0.85f,
            0.20f,
            1.0f
        };

        /**
         * @brief Color assigned to hovered edges.
         */
        graphics::ColorRGBA hoveredEdgeColor{
            1.0f,
            0.85f,
            0.20f,
            1.0f
        };

        /**
         * @brief Color assigned to hovered faces.
         */
        graphics::ColorRGBA hoveredFaceColor{
            1.0f,
            0.85f,
            0.20f,
            0.30f
        };
    };

    /**
     * @brief Diagnostics produced while building one mesh overlay.
     */
    struct OverlayRenderResult {
        /**
         * @brief Source mesh node identifier.
         */
        SceneNodeId nodeId{};

        /**
         * @brief Number of active mesh vertices inspected.
         */
        std::size_t visitedVertexCount = 0;

        /**
         * @brief Number of active mesh edges inspected.
         */
        std::size_t visitedEdgeCount = 0;

        /**
         * @brief Number of active mesh faces inspected.
         */
        std::size_t visitedFaceCount = 0;

        /**
         * @brief Number of primitive groups emitted.
         */
        std::size_t groupCount = 0;

        /**
         * @brief Number of point vertices generated.
         */
        std::size_t pointVertexCount = 0;

        /**
         * @brief Number of line vertices generated.
         */
        std::size_t lineVertexCount = 0;

        /**
         * @brief Number of triangle vertices generated.
         */
        std::size_t triangleVertexCount = 0;

        /**
         * @brief Number of invalid selected or hovered handles ignored.
         */
        std::size_t invalidHandleCount = 0;

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message;

        /**
         * @brief Checks whether any primitive group was generated.
         *
         * @return True when at least one group was emitted.
         */
        [[nodiscard]] bool has_geometry() const {
            return groupCount > 0;
        }
    };

    /**
     * @brief Converts editor mesh state into generic graphical overlay geometry.
     *
     * OverlayRenderAdapter owns the semantic conversion from MeshNode and
     * MeshSelection into graphics primitive meshes. It does not upload GPU
     * resources, create RenderObjects, access OpenGL, or synchronize RenderScene.
     */
    class OverlayRenderAdapter {
    public:
        /**
         * @brief Builds object-space overlay geometry for one mesh node.
         *
         * Component selection is applied only when the selection active mesh
         * matches the supplied node identifier.
         *
         * @param node Source editor mesh node.
         * @param selection Editor mesh component selection state.
         * @param options Overlay generation options.
         * @param result Optional diagnostic output.
         * @return Generated object-space overlay geometry.
         */
        [[nodiscard]] static OverlayGeometry build_mesh_overlay(
            const MeshNode& node,
            const MeshSelection& selection,
            const OverlayRenderOptions& options = {},
            OverlayRenderResult* result = nullptr
        );
    };

} // namespace locus::editor