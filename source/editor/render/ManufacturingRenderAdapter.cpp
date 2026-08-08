/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/ManufacturingRenderAdapter.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/sync/ManufacturingSync.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/core/PrintIssue.h"

#include <algorithm>
#include <map>
#include <utility>

#include <glm/vec4.hpp>

namespace locus::editor {

    namespace {

        using kernel::geometry::EdgeHandle;
        using kernel::geometry::FaceHandle;
        using kernel::geometry::LEM;
        using kernel::geometry::MeshTriangulator;
        using kernel::geometry::RenderMesh;
        using kernel::geometry::RenderTriangle;
        using kernel::geometry::RenderVertex;
        using kernel::geometry::TopologyTraversal;
        using kernel::geometry::VertexHandle;
        using kernel::manufacturing::PrintIssue;

        struct SelectedStyle {
            ManufacturingVisualStyle style{};
            bool valid = false;
        };

        [[nodiscard]] glm::vec3 transform_point(
            const glm::mat4& matrix,
            const glm::vec3& point)
        {
            return glm::vec3{ matrix * glm::vec4{ point, 1.0f } };
        }

        [[nodiscard]] graphics::ColorRGBA with_alpha(
            graphics::ColorRGBA color,
            float alpha) noexcept
        {
            color.a *= alpha;
            return color;
        }

        void select_style(
            std::map<kernel::IdValue, SelectedStyle>& styles,
            kernel::IdValue key,
            const ManufacturingVisualStyle& style)
        {
            SelectedStyle& selected = styles[key];
            if (!selected.valid || style.priority > selected.style.priority) {
                selected.style = style;
                selected.valid = true;
            }
        }

        void append_face_surface(
            graphics::SurfaceOverlayBatch& batch,
            const LEM& mesh,
            FaceHandle face,
            const glm::mat4& worldMatrix,
            const ManufacturingVisualStyle& style,
            ManufacturingRenderResult* result)
        {
            RenderMesh renderMesh{};
            MeshTriangulator::triangulate_face_into(mesh, face, renderMesh);

            if (renderMesh.triangles.empty()) {
                return;
            }

            const std::uint32_t base =
                static_cast<std::uint32_t>(batch.vertices.size());
            batch.vertices.reserve(batch.vertices.size() + renderMesh.vertices.size());
            batch.indices.reserve(batch.indices.size() + renderMesh.triangles.size() * 3u);

            const graphics::ColorRGBA color =
                with_alpha(style.color, style.surfaceAlpha);

            for (const RenderVertex& vertex : renderMesh.vertices) {
                graphics::SurfaceOverlayVertex overlayVertex{};
                overlayVertex.position = transform_point(worldMatrix, vertex.position);
                overlayVertex.color = color;
                batch.vertices.push_back(overlayVertex);
            }

            for (const RenderTriangle& triangle : renderMesh.triangles) {
                if (triangle.a >= renderMesh.vertices.size() ||
                    triangle.b >= renderMesh.vertices.size() ||
                    triangle.c >= renderMesh.vertices.size()) {
                    if (result != nullptr) {
                        ++result->invalidHandleCount;
                    }
                    continue;
                }

                batch.indices.push_back(base + triangle.a);
                batch.indices.push_back(base + triangle.b);
                batch.indices.push_back(base + triangle.c);

                if (result != nullptr) {
                    ++result->surfaceTriangleCount;
                }
            }
        }

        void append_edge_line(
            graphics::ScreenSpaceLineBatch& batch,
            const LEM& mesh,
            EdgeHandle edge,
            const glm::mat4& worldMatrix,
            const ManufacturingVisualStyle& style,
            ManufacturingRenderResult* result)
        {
            if (!mesh.is_valid(edge)) {
                if (result != nullptr) {
                    ++result->invalidHandleCount;
                }
                return;
            }

            const auto vertices = TopologyTraversal::edge_vertices(mesh, edge);
            if (!mesh.is_valid(vertices[0]) || !mesh.is_valid(vertices[1])) {
                if (result != nullptr) {
                    ++result->invalidHandleCount;
                }
                return;
            }

            graphics::ScreenSpaceLine line{};
            line.start = transform_point(worldMatrix, mesh.vertex(vertices[0]).position);
            line.end = transform_point(worldMatrix, mesh.vertex(vertices[1]).position);
            line.color = with_alpha(style.color, style.lineAlpha);
            line.widthPixels = style.lineWidthPixels;
            batch.lines.push_back(line);

            if (result != nullptr) {
                ++result->lineCount;
            }
        }

        void append_vertex_marker(
            graphics::PointMarkerBatch& batch,
            const LEM& mesh,
            VertexHandle vertex,
            const glm::mat4& worldMatrix,
            const ManufacturingVisualStyle& style,
            const ManufacturingDisplaySettings& settings,
            ManufacturingRenderResult* result)
        {
            if (!mesh.is_valid(vertex)) {
                if (result != nullptr) {
                    ++result->invalidHandleCount;
                }
                return;
            }

            graphics::PointMarker marker{};
            marker.position = transform_point(worldMatrix, mesh.vertex(vertex).position);
            marker.fillColor = with_alpha(style.color, style.markerAlpha);
            marker.borderColor = settings.markerBorderColor;
            marker.radiusPixels = style.markerRadiusPixels;
            marker.borderWidthPixels = style.markerBorderWidthPixels;
            batch.markers.push_back(marker);

            if (result != nullptr) {
                ++result->markerCount;
            }
        }

        void append_sample_marker(
            graphics::PointMarkerBatch& batch,
            const glm::vec3& position,
            const ManufacturingVisualStyle& style,
            const ManufacturingDisplaySettings& settings,
            ManufacturingRenderResult* result)
        {
            graphics::PointMarker marker{};
            marker.position = position;
            marker.fillColor = with_alpha(style.color, style.markerAlpha);
            marker.borderColor = settings.markerBorderColor;
            marker.radiusPixels = style.markerRadiusPixels;
            marker.borderWidthPixels = style.markerBorderWidthPixels;
            batch.markers.push_back(marker);

            if (result != nullptr) {
                ++result->markerCount;
            }
        }

    } // namespace

    ManufacturingOverlayBatches ManufacturingRenderAdapter::build_overlays(
        const EditorScene& scene,
        const ManufacturingSync& manufacturing,
        ManufacturingRenderResult* result)
    {
        if (result != nullptr) {
            *result = {};
        }

        ManufacturingOverlayBatches batches{};
        batches.surfaces.modelMatrix = glm::mat4{ 1.0f };

        const ManufacturingDisplaySettings& settings =
            manufacturing.display_settings();
        if (!settings.enabled) {
            if (result != nullptr) {
                result->message = "Manufacturing overlays skipped because display is disabled.";
            }
            return batches;
        }

        const std::vector<const ManufacturingNodeResult*> results =
            manufacturing.results();

        for (const ManufacturingNodeResult* nodeResult : results) {
            if (nodeResult == nullptr || !nodeResult->valid) {
                continue;
            }

            const MeshNode* meshNode = scene.find_mesh(nodeResult->nodeId);
            if (meshNode == nullptr) {
                continue;
            }

            if (result != nullptr) {
                ++result->nodeCount;
            }

            const LEM& mesh = meshNode->mesh();
            std::map<kernel::IdValue, SelectedStyle> faceStyles;
            std::map<kernel::IdValue, SelectedStyle> edgeStyles;
            std::map<kernel::IdValue, SelectedStyle> vertexStyles;

            for (const PrintIssue& issue : nodeResult->report.issues()) {
                if (result != nullptr) {
                    ++result->issueCount;
                }

                if (!settings.is_type_visible(issue.type) ||
                    !settings.is_severity_visible(issue.severity)) {
                    if (result != nullptr) {
                        ++result->skippedIssueCount;
                    }
                    continue;
                }

                const ManufacturingVisualStyle style =
                    settings.style_for(issue.type);

                for (const FaceHandle face : issue.location.faces) {
                    select_style(faceStyles, face.id.value, style);
                }

                for (const EdgeHandle edge : issue.location.edges) {
                    select_style(edgeStyles, edge.id.value, style);
                }

                for (const VertexHandle vertex : issue.location.vertices) {
                    select_style(vertexStyles, vertex.id.value, style);
                }

                for (const glm::vec3& sample : issue.location.samples) {
                    append_sample_marker(
                        batches.markers,
                        sample,
                        style,
                        settings,
                        result);
                }
            }

            for (const auto& entry : faceStyles) {
                append_face_surface(
                    batches.surfaces,
                    mesh,
                    FaceHandle{ entry.first },
                    nodeResult->worldMatrix,
                    entry.second.style,
                    result);
            }

            for (const auto& entry : edgeStyles) {
                append_edge_line(
                    batches.lines,
                    mesh,
                    EdgeHandle{ entry.first },
                    nodeResult->worldMatrix,
                    entry.second.style,
                    result);
            }

            for (const auto& entry : vertexStyles) {
                append_vertex_marker(
                    batches.markers,
                    mesh,
                    VertexHandle{ entry.first },
                    nodeResult->worldMatrix,
                    entry.second.style,
                    settings,
                    result);
            }
        }

        if (result != nullptr) {
            result->message = batches.empty()
                ? "Manufacturing analysis produced no drawable overlays."
                : "Manufacturing overlays built successfully.";
        }

        return batches;
    }

} // namespace locus::editor
