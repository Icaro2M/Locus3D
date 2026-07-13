/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/OverlayRenderAdapter.h"

#include "editor/scene/MeshNode.h"
#include "editor/selection/MeshSelection.h"
#include "graphics/primitives/PrimitiveBuilder.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        using locus::graphics::ColorRGBA;
        using locus::graphics::PrimitiveBuilder;
        using locus::graphics::PrimitiveMesh;
        using locus::graphics::PrimitiveTopology;

        using locus::kernel::geometry::Edge;
        using locus::kernel::geometry::EdgeHandle;
        using locus::kernel::geometry::Face;
        using locus::kernel::geometry::FaceHandle;
        using locus::kernel::geometry::LEM;
        using locus::kernel::geometry::MeshTriangulator;
        using locus::kernel::geometry::RenderMesh;
        using locus::kernel::geometry::RenderTriangle;
        using locus::kernel::geometry::RenderVertex;
        using locus::kernel::geometry::TopologyTraversal;
        using locus::kernel::geometry::Vertex;
        using locus::kernel::geometry::VertexHandle;

        /**
         * @brief Updates diagnostic primitive vertex counters.
         *
         * @param mesh Primitive mesh to inspect.
         * @param result Optional diagnostic output.
         */
        void count_primitive_vertices(
            const PrimitiveMesh& mesh,
            OverlayRenderResult* result
        ) {
            if (!result) {
                return;
            }

            switch (mesh.topology) {
            case PrimitiveTopology::Points:
                result->pointVertexCount += mesh.vertices.size();
                break;

            case PrimitiveTopology::Lines:
            case PrimitiveTopology::LineStrip:
                result->lineVertexCount += mesh.vertices.size();
                break;

            case PrimitiveTopology::Triangles:
            case PrimitiveTopology::TriangleStrip:
                result->triangleVertexCount += mesh.vertices.size();
                break;
            }
        }

        /**
         * @brief Appends a non-empty primitive group to an overlay result.
         *
         * @param geometry Overlay geometry receiving the group.
         * @param role Semantic role assigned to the group.
         * @param mesh Primitive geometry to append.
         * @param result Optional diagnostic output.
         */
        void append_group(
            OverlayGeometry& geometry,
            const OverlayPrimitiveRole role,
            PrimitiveMesh mesh,
            OverlayRenderResult* result
        ) {
            if (!mesh.is_valid()) {
                return;
            }

            count_primitive_vertices(mesh, result);

            OverlayPrimitiveGroup group{};
            group.role = role;
            group.mesh = std::move(mesh);

            geometry.groups.push_back(std::move(group));

            if (result) {
                ++result->groupCount;
            }
        }

        /**
         * @brief Checks whether a vertex should be skipped from visible overlays.
         *
         * @param mesh Source editable mesh.
         * @param handle Vertex handle.
         * @param skipHidden Whether hidden elements should be skipped.
         * @return True when the vertex should not be emitted.
         */
        [[nodiscard]] bool should_skip_vertex(
            const LEM& mesh,
            const VertexHandle handle,
            const bool skipHidden
        ) {
            if (!mesh.is_valid(handle)) {
                return true;
            }

            return skipHidden && mesh.vertex(handle).hidden;
        }

        /**
         * @brief Checks whether an edge should be skipped from visible overlays.
         *
         * @param mesh Source editable mesh.
         * @param handle Edge handle.
         * @param skipHidden Whether hidden elements should be skipped.
         * @return True when the edge should not be emitted.
         */
        [[nodiscard]] bool should_skip_edge(
            const LEM& mesh,
            const EdgeHandle handle,
            const bool skipHidden
        ) {
            if (!mesh.is_valid(handle)) {
                return true;
            }

            return skipHidden && mesh.edge(handle).hidden;
        }

        /**
         * @brief Checks whether a face should be skipped from visible overlays.
         *
         * @param mesh Source editable mesh.
         * @param handle Face handle.
         * @param skipHidden Whether hidden elements should be skipped.
         * @return True when the face should not be emitted.
         */
        [[nodiscard]] bool should_skip_face(
            const LEM& mesh,
            const FaceHandle handle,
            const bool skipHidden
        ) {
            if (!mesh.is_valid(handle)) {
                return true;
            }

            return skipHidden && mesh.face(handle).hidden;
        }

        /**
         * @brief Adds one editable mesh vertex to a point builder.
         *
         * @param builder Point primitive builder.
         * @param mesh Source editable mesh.
         * @param handle Vertex handle.
         * @param color Overlay color.
         * @param skipHidden Whether hidden vertices should be skipped.
         * @return True when the vertex was emitted.
         */
        bool append_vertex(
            PrimitiveBuilder& builder,
            const LEM& mesh,
            const VertexHandle handle,
            const ColorRGBA& color,
            const bool skipHidden
        ) {
            if (should_skip_vertex(mesh, handle, skipHidden)) {
                return false;
            }

            const Vertex& vertex = mesh.vertex(handle);
            return builder.add_point(vertex.position, color);
        }

        /**
         * @brief Adds one editable mesh edge to a line builder.
         *
         * @param builder Line primitive builder.
         * @param mesh Source editable mesh.
         * @param handle Edge handle.
         * @param color Overlay color.
         * @param skipHidden Whether hidden edges should be skipped.
         * @return True when the edge was emitted.
         */
        bool append_edge(
            PrimitiveBuilder& builder,
            const LEM& mesh,
            const EdgeHandle handle,
            const ColorRGBA& color,
            const bool skipHidden
        ) {
            if (should_skip_edge(mesh, handle, skipHidden)) {
                return false;
            }

            const Edge& edge = mesh.edge(handle);

            if (!mesh.is_valid(edge.vertexA)
                || !mesh.is_valid(edge.vertexB)) {
                return false;
            }

            const Vertex& vertexA = mesh.vertex(edge.vertexA);
            const Vertex& vertexB = mesh.vertex(edge.vertexB);

            return builder.add_line(
                vertexA.position,
                vertexB.position,
                color
            );
        }

        /**
         * @brief Adds one triangulated editable mesh face to a triangle builder.
         *
         * Face triangulation remains delegated to the geometry kernel.
         *
         * @param builder Triangle primitive builder.
         * @param mesh Source editable mesh.
         * @param handle Face handle.
         * @param color Overlay color.
         * @param skipHidden Whether hidden faces should be skipped.
         * @return True when at least one triangle was emitted.
         */
        bool append_face(
            PrimitiveBuilder& builder,
            const LEM& mesh,
            const FaceHandle handle,
            const ColorRGBA& color,
            const bool skipHidden
        ) {
            if (should_skip_face(mesh, handle, skipHidden)) {
                return false;
            }

            RenderMesh renderMesh{};

            MeshTriangulator::triangulate_face_into(
                mesh,
                handle,
                renderMesh
            );

            bool emitted = false;

            for (const RenderTriangle& triangle : renderMesh.triangles) {
                if (triangle.a >= renderMesh.vertices.size()
                    || triangle.b >= renderMesh.vertices.size()
                    || triangle.c >= renderMesh.vertices.size()) {
                    continue;
                }

                const RenderVertex& a = renderMesh.vertices[triangle.a];
                const RenderVertex& b = renderMesh.vertices[triangle.b];
                const RenderVertex& c = renderMesh.vertices[triangle.c];

                graphics::PrimitiveVertex primitiveA{};
                primitiveA.position = a.position;
                primitiveA.normal = a.normal;
                primitiveA.color = color;

                graphics::PrimitiveVertex primitiveB{};
                primitiveB.position = b.position;
                primitiveB.normal = b.normal;
                primitiveB.color = color;

                graphics::PrimitiveVertex primitiveC{};
                primitiveC.position = c.position;
                primitiveC.normal = c.normal;
                primitiveC.color = color;

                emitted |= builder.add_triangle(
                    primitiveA,
                    primitiveB,
                    primitiveC
                );
            }

            return emitted;
        }

        /**
         * @brief Builds the complete topological wireframe.
         *
         * @param mesh Source editable mesh.
         * @param options Overlay generation options.
         * @return Line primitive mesh containing active visible edges.
         */
        [[nodiscard]] PrimitiveMesh build_wireframe(
            const LEM& mesh,
            const OverlayRenderOptions& options
        ) {
            PrimitiveBuilder builder{ PrimitiveTopology::Lines };

            for (const EdgeHandle handle : TopologyTraversal::edges(mesh)) {
                append_edge(
                    builder,
                    mesh,
                    handle,
                    options.wireframeColor,
                    options.skipHiddenComponents
                );
            }

            return builder.build();
        }

        /**
         * @brief Builds selected vertex overlay geometry.
         *
         * @param mesh Source editable mesh.
         * @param selection Mesh component selection.
         * @param options Overlay generation options.
         * @param result Optional diagnostic output.
         * @return Point primitive mesh containing selected vertices.
         */
        [[nodiscard]] PrimitiveMesh build_selected_vertices(
            const LEM& mesh,
            const MeshSelection& selection,
            const OverlayRenderOptions& options,
            OverlayRenderResult* result
        ) {
            PrimitiveBuilder builder{ PrimitiveTopology::Points };

            for (const VertexHandle handle
                : selection.vertices().items()) {
                if (!mesh.is_valid(handle)) {
                    if (result) {
                        ++result->invalidHandleCount;
                    }

                    continue;
                }

                append_vertex(
                    builder,
                    mesh,
                    handle,
                    options.selectedVertexColor,
                    options.skipHiddenComponents
                );
            }

            return builder.build();
        }

        /**
         * @brief Builds selected edge overlay geometry.
         *
         * @param mesh Source editable mesh.
         * @param selection Mesh component selection.
         * @param options Overlay generation options.
         * @param result Optional diagnostic output.
         * @return Line primitive mesh containing selected edges.
         */
        [[nodiscard]] PrimitiveMesh build_selected_edges(
            const LEM& mesh,
            const MeshSelection& selection,
            const OverlayRenderOptions& options,
            OverlayRenderResult* result
        ) {
            PrimitiveBuilder builder{ PrimitiveTopology::Lines };

            for (const EdgeHandle handle : selection.edges().items()) {
                if (!mesh.is_valid(handle)) {
                    if (result) {
                        ++result->invalidHandleCount;
                    }

                    continue;
                }

                append_edge(
                    builder,
                    mesh,
                    handle,
                    options.selectedEdgeColor,
                    options.skipHiddenComponents
                );
            }

            return builder.build();
        }

        /**
         * @brief Builds selected face overlay geometry.
         *
         * @param mesh Source editable mesh.
         * @param selection Mesh component selection.
         * @param options Overlay generation options.
         * @param result Optional diagnostic output.
         * @return Triangle primitive mesh containing selected faces.
         */
        [[nodiscard]] PrimitiveMesh build_selected_faces(
            const LEM& mesh,
            const MeshSelection& selection,
            const OverlayRenderOptions& options,
            OverlayRenderResult* result
        ) {
            PrimitiveBuilder builder{
                PrimitiveTopology::Triangles
            };

            for (const FaceHandle handle : selection.faces().items()) {
                if (!mesh.is_valid(handle)) {
                    if (result) {
                        ++result->invalidHandleCount;
                    }

                    continue;
                }

                append_face(
                    builder,
                    mesh,
                    handle,
                    options.selectedFaceColor,
                    options.skipHiddenComponents
                );
            }

            return builder.build();
        }

        /**
         * @brief Builds hovered vertex overlay geometry.
         *
         * @param mesh Source editable mesh.
         * @param selection Mesh component selection.
         * @param options Overlay generation options.
         * @param result Optional diagnostic output.
         * @return Point primitive mesh containing the hovered vertex.
         */
        [[nodiscard]] PrimitiveMesh build_hovered_vertex(
            const LEM& mesh,
            const MeshSelection& selection,
            const OverlayRenderOptions& options,
            OverlayRenderResult* result
        ) {
            PrimitiveBuilder builder{ PrimitiveTopology::Points };

            const VertexHandle handle = selection.hovered_vertex();

            if (!handle.is_valid()) {
                return builder.build();
            }

            if (!mesh.is_valid(handle)) {
                if (result) {
                    ++result->invalidHandleCount;
                }

                return builder.build();
            }

            append_vertex(
                builder,
                mesh,
                handle,
                options.hoveredVertexColor,
                options.skipHiddenComponents
            );

            return builder.build();
        }

        /**
         * @brief Builds hovered edge overlay geometry.
         *
         * @param mesh Source editable mesh.
         * @param selection Mesh component selection.
         * @param options Overlay generation options.
         * @param result Optional diagnostic output.
         * @return Line primitive mesh containing the hovered edge.
         */
        [[nodiscard]] PrimitiveMesh build_hovered_edge(
            const LEM& mesh,
            const MeshSelection& selection,
            const OverlayRenderOptions& options,
            OverlayRenderResult* result
        ) {
            PrimitiveBuilder builder{ PrimitiveTopology::Lines };

            const EdgeHandle handle = selection.hovered_edge();

            if (!handle.is_valid()) {
                return builder.build();
            }

            if (!mesh.is_valid(handle)) {
                if (result) {
                    ++result->invalidHandleCount;
                }

                return builder.build();
            }

            append_edge(
                builder,
                mesh,
                handle,
                options.hoveredEdgeColor,
                options.skipHiddenComponents
            );

            return builder.build();
        }

        /**
         * @brief Builds hovered face overlay geometry.
         *
         * @param mesh Source editable mesh.
         * @param selection Mesh component selection.
         * @param options Overlay generation options.
         * @param result Optional diagnostic output.
         * @return Triangle primitive mesh containing the hovered face.
         */
        [[nodiscard]] PrimitiveMesh build_hovered_face(
            const LEM& mesh,
            const MeshSelection& selection,
            const OverlayRenderOptions& options,
            OverlayRenderResult* result
        ) {
            PrimitiveBuilder builder{
                PrimitiveTopology::Triangles
            };

            const FaceHandle handle = selection.hovered_face();

            if (!handle.is_valid()) {
                return builder.build();
            }

            if (!mesh.is_valid(handle)) {
                if (result) {
                    ++result->invalidHandleCount;
                }

                return builder.build();
            }

            append_face(
                builder,
                mesh,
                handle,
                options.hoveredFaceColor,
                options.skipHiddenComponents
            );

            return builder.build();
        }

    } // namespace

    OverlayGeometry OverlayRenderAdapter::build_mesh_overlay(
        const MeshNode& node,
        const MeshSelection& selection,
        const OverlayRenderOptions& options,
        OverlayRenderResult* result
    ) {
        if (result) {
            *result = {};
            result->nodeId = node.id();
        }

        OverlayGeometry geometry{};
        geometry.nodeId = node.id();

        if (node.id().is_invalid()) {
            if (result) {
                result->message =
                    "Cannot build overlay for mesh node with invalid id.";
            }

            return geometry;
        }

        const LEM& mesh = node.mesh();

        const std::vector<VertexHandle> activeVertices =
            TopologyTraversal::vertices(mesh);

        const std::vector<EdgeHandle> activeEdges =
            TopologyTraversal::edges(mesh);

        const std::vector<FaceHandle> activeFaces =
            TopologyTraversal::faces(mesh);

        if (result) {
            result->visitedVertexCount = activeVertices.size();
            result->visitedEdgeCount = activeEdges.size();
            result->visitedFaceCount = activeFaces.size();
        }

        if (mesh.empty()) {
            if (result) {
                result->message =
                    "Mesh node produced no overlay geometry because its mesh is empty.";
            }

            return geometry;
        }

        if (options.includeWireframe) {
            append_group(
                geometry,
                OverlayPrimitiveRole::Wireframe,
                build_wireframe(mesh, options),
                result
            );
        }

        const bool selectionMatchesNode =
            selection.active_mesh() == node.id();

        if (selectionMatchesNode) {
            if (options.includeSelectedVertices) {
                append_group(
                    geometry,
                    OverlayPrimitiveRole::SelectedVertices,
                    build_selected_vertices(
                        mesh,
                        selection,
                        options,
                        result
                    ),
                    result
                );
            }

            if (options.includeSelectedEdges) {
                append_group(
                    geometry,
                    OverlayPrimitiveRole::SelectedEdges,
                    build_selected_edges(
                        mesh,
                        selection,
                        options,
                        result
                    ),
                    result
                );
            }

            if (options.includeSelectedFaces) {
                append_group(
                    geometry,
                    OverlayPrimitiveRole::SelectedFaces,
                    build_selected_faces(
                        mesh,
                        selection,
                        options,
                        result
                    ),
                    result
                );
            }

            if (options.includeHoveredVertex) {
                append_group(
                    geometry,
                    OverlayPrimitiveRole::HoveredVertex,
                    build_hovered_vertex(
                        mesh,
                        selection,
                        options,
                        result
                    ),
                    result
                );
            }

            if (options.includeHoveredEdge) {
                append_group(
                    geometry,
                    OverlayPrimitiveRole::HoveredEdge,
                    build_hovered_edge(
                        mesh,
                        selection,
                        options,
                        result
                    ),
                    result
                );
            }

            if (options.includeHoveredFace) {
                append_group(
                    geometry,
                    OverlayPrimitiveRole::HoveredFace,
                    build_hovered_face(
                        mesh,
                        selection,
                        options,
                        result
                    ),
                    result
                );
            }
        }

        if (result) {
            if (!geometry.has_geometry()) {
                result->message =
                    "Mesh node produced no drawable overlay geometry.";
            }
            else if (!selectionMatchesNode) {
                result->message =
                    "Mesh wireframe overlay built without component selection.";
            }
            else {
                result->message =
                    "Mesh overlay geometry built successfully.";
            }
        }

        return geometry;
    }

} // namespace locus::editor