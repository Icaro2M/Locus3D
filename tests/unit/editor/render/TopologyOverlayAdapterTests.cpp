/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorRenderTestSuite.h"

#include "editor/render/TopologyOverlayAdapter.h"
#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <vector>

namespace {

constexpr float epsilon = 0.0001f;

struct QuadHandles {
    locus::kernel::geometry::VertexHandle v0{};
    locus::kernel::geometry::VertexHandle v1{};
    locus::kernel::geometry::VertexHandle v2{};
    locus::kernel::geometry::VertexHandle v3{};
    locus::kernel::geometry::EdgeHandle bottom{};
    locus::kernel::geometry::EdgeHandle right{};
    locus::kernel::geometry::EdgeHandle top{};
    locus::kernel::geometry::EdgeHandle left{};
};

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] bool near_mat4_translation(
    const glm::mat4& matrix,
    const glm::vec3& expected)
{
    const glm::vec4 transformed =
        matrix * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
    return near_vec3(glm::vec3{ transformed }, expected);
}

[[nodiscard]] bool near_color(
    const locus::graphics::ColorRGBA& lhs,
    const locus::graphics::ColorRGBA& rhs)
{
    return near(lhs.r, rhs.r) &&
        near(lhs.g, rhs.g) &&
        near(lhs.b, rhs.b) &&
        near(lhs.a, rhs.a);
}

[[nodiscard]] QuadHandles add_quad(locus::kernel::geometry::LEM& mesh)
{
    using namespace locus::kernel::geometry;

    const VertexHandle v0 = mesh.add_vertex({ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex({ 1.0f, 0.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex({ 1.0f, 1.0f, 0.0f });
    const VertexHandle v3 = mesh.add_vertex({ 0.0f, 1.0f, 0.0f });

    mesh.add_face({ v0, v1, v2, v3 });

    return {
        v0,
        v1,
        v2,
        v3,
        mesh.find_edge(v0, v1),
        mesh.find_edge(v1, v2),
        mesh.find_edge(v2, v3),
        mesh.find_edge(v3, v0)
    };
}

[[nodiscard]] locus::kernel::geometry::FaceHandle first_face(
    const locus::kernel::geometry::LEM& mesh)
{
    const std::vector<locus::kernel::geometry::FaceHandle> faces =
        locus::kernel::geometry::TopologyTraversal::faces(mesh);

    return faces.empty()
        ? locus::kernel::geometry::FaceHandle{}
        : faces.front();
}

[[nodiscard]] locus::kernel::geometry::FaceHandle add_triangle(
    locus::kernel::geometry::LEM& mesh)
{
    using namespace locus::kernel::geometry;

    const VertexHandle v0 = mesh.add_vertex({ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex({ 1.0f, 0.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex({ 0.0f, 1.0f, 0.0f });

    return mesh.add_face({ v0, v1, v2 });
}

[[nodiscard]] locus::kernel::geometry::FaceHandle add_ngon(
    locus::kernel::geometry::LEM& mesh)
{
    using namespace locus::kernel::geometry;

    const VertexHandle v0 = mesh.add_vertex({ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex({ 1.0f, 0.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex({ 1.25f, 0.75f, 0.0f });
    const VertexHandle v3 = mesh.add_vertex({ 0.5f, 1.25f, 0.0f });
    const VertexHandle v4 = mesh.add_vertex({ -0.25f, 0.75f, 0.0f });

    return mesh.add_face({ v0, v1, v2, v3, v4 });
}

[[nodiscard]] locus::editor::SceneNodeId make_quad_scene(
    locus::editor::EditorScene& scene,
    QuadHandles& handles)
{
    const locus::editor::SceneNodeId meshId = scene.create_mesh("Quad");
    handles = add_quad(scene.find_mesh(meshId)->mesh());
    return meshId;
}

} // namespace

namespace locus::tests {

TestResult run_topology_overlay_adapter_tests()
{
    editor::EditorScene scene;
    QuadHandles handles{};
    const editor::SceneNodeId meshId = make_quad_scene(scene, handles);

    editor::SelectionState selection;
    selection.set_scope(editor::SelectionScope::ActiveMesh);
    selection.set_granularity(editor::SelectionGranularity::Edge);
    selection.mesh().set_active_mesh(meshId);

    graphics::TopologyOverlayStyle style{};
    style.wireframeColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    style.hoveredEdgeColor = { 0.4f, 0.5f, 0.6f, 1.0f };
    style.selectedEdgeColor = { 0.7f, 0.8f, 0.9f, 1.0f };
    style.wireframeWidthPixels = 1.0f;
    style.hoveredEdgeWidthPixels = 2.0f;
    style.selectedEdgeWidthPixels = 3.0f;

    editor::TopologyOverlayOptions options;
    options.style = style;

    editor::TopologyOverlayResult result;
    graphics::ScreenSpaceLineBatch batch =
        editor::TopologyOverlayAdapter::build_active_mesh_lines(
            scene,
            selection,
            options,
            &result);

    if (batch.size() != 4u ||
        result.visitedEdgeCount != 4u ||
        result.wireframeEdgeCount != 4u ||
        result.hoveredEdgeCount != 0u ||
        result.selectedEdgeCount != 0u) {
        return TestResult::fail("quad topology overlay should produce four wireframe edges and no diagonal");
    }

    selection.mesh().add_edge(handles.bottom);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, selection, options, &result);
    if (result.selectedEdgeCount != 1u || result.wireframeEdgeCount != 3u) {
        return TestResult::fail("topology overlay should classify one selected edge");
    }

    selection.mesh().set_hovered_edge(handles.right);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, selection, options, &result);
    if (result.hoveredEdgeCount != 1u ||
        result.selectedEdgeCount != 1u ||
        result.wireframeEdgeCount != 2u) {
        return TestResult::fail("topology overlay should classify one hovered edge");
    }

    selection.mesh().set_hovered_edge(handles.bottom);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, selection, options, &result);
    if (result.hoveredEdgeCount != 0u || result.selectedEdgeCount != 1u) {
        return TestResult::fail("selected edge should take precedence over hover");
    }

    selection.mesh().add_edge(handles.top);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, selection, options, &result);
    if (result.selectedEdgeCount != 2u) {
        return TestResult::fail("topology overlay should support multi-selection");
    }

    bool sawSelectedStyle = false;
    for (const graphics::ScreenSpaceLine& line : batch.lines) {
        sawSelectedStyle = sawSelectedStyle ||
            (near(line.widthPixels, style.selectedEdgeWidthPixels) &&
                near_color(line.color, style.selectedEdgeColor));
    }

    if (!sawSelectedStyle) {
        return TestResult::fail("topology overlay should apply selected edge style");
    }

    selection.mesh().add_edge(locus::kernel::geometry::EdgeHandle{ 500 });
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, selection, options, &result);
    if (result.invalidHandleCount != 1u || result.selectedEdgeCount != 2u) {
        return TestResult::fail("invalid selected edge handles should be diagnosed without emitting lines");
    }

    const editor::SceneNodeId parent = scene.create_empty("Parent");
    scene.find_node(parent)->transform().set_position({ 2.0f, 0.0f, 0.0f });
    scene.find_node(meshId)->transform().set_position({ 0.0f, 3.0f, 0.0f });
    scene.reparent(meshId, parent);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, selection, options, &result);
    if (batch.empty() ||
        !near_vec3(batch.lines.front().start, { 2.0f, 3.0f, 0.0f }) ||
        !near_vec3(batch.lines.front().end, { 3.0f, 3.0f, 0.0f })) {
        return TestResult::fail("topology overlay should apply parent and node transforms once");
    }

    const editor::SceneNodeId ngonMesh = scene.create_mesh("Ngon wireframe");
    add_ngon(scene.find_mesh(ngonMesh)->mesh());
    selection.mesh().set_active_mesh(ngonMesh);
    selection.mesh().clear_components();
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, selection, options, &result);
    if (batch.size() != 5u ||
        result.visitedEdgeCount != 5u ||
        result.wireframeEdgeCount != 5u) {
        return TestResult::fail("n-gon topology overlay should emit boundary edges without triangulation diagonals");
    }

    editor::SelectionState noActiveMesh;
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, noActiveMesh, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("topology overlay should return an empty batch without an active mesh");
    }

    editor::EditorScene visibleScene;
    QuadHandles visibleA{};
    QuadHandles visibleB{};
    const editor::SceneNodeId visibleMeshA =
        make_quad_scene(visibleScene, visibleA);
    make_quad_scene(visibleScene, visibleB);
    const editor::SceneNodeId hiddenMesh =
        visibleScene.create_mesh("Hidden");
    add_triangle(visibleScene.find_mesh(hiddenMesh)->mesh());
    visibleScene.find_node(hiddenMesh)->metadata().visible = false;

    editor::SelectionState visibleSelection;
    visibleSelection.set_scope(editor::SelectionScope::ActiveMesh);
    visibleSelection.set_granularity(editor::SelectionGranularity::Edge);
    visibleSelection.mesh().set_active_mesh(visibleMeshA);
    visibleSelection.mesh().add_edge(visibleA.bottom);

    batch = editor::TopologyOverlayAdapter::build_visible_mesh_lines(
        visibleScene,
        visibleSelection,
        options,
        &result);

    if (batch.size() != 8u ||
        result.visitedEdgeCount != 8u ||
        result.selectedEdgeCount != 1u ||
        result.wireframeEdgeCount != 7u) {
        return TestResult::fail("visible mesh topology should include all visible mesh edges and skip hidden meshes");
    }

    editor::SelectionState noVisibleActive;
    batch = editor::TopologyOverlayAdapter::build_visible_mesh_lines(
        visibleScene,
        noVisibleActive,
        options,
        &result);
    if (batch.size() != 8u || result.wireframeEdgeCount != 8u) {
        return TestResult::fail("visible mesh topology should not require an active mesh");
    }

    editor::EditorScene emptyScene;
    const editor::SceneNodeId emptyMesh = emptyScene.create_mesh("Empty");
    editor::SelectionState emptySelection;
    emptySelection.mesh().set_active_mesh(emptyMesh);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(emptyScene, emptySelection, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("topology overlay should return an empty batch for an empty mesh");
    }

    return TestResult::pass();
}

TestResult run_topology_overlay_vertex_marker_tests()
{
    editor::EditorScene scene;
    QuadHandles handles{};
    const editor::SceneNodeId meshId = make_quad_scene(scene, handles);

    editor::SelectionState selection;
    selection.set_scope(editor::SelectionScope::ActiveMesh);
    selection.set_granularity(editor::SelectionGranularity::Vertex);
    selection.mesh().set_active_mesh(meshId);

    graphics::TopologyOverlayStyle style{};
    style.vertexColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    style.hoveredVertexColor = { 0.4f, 0.5f, 0.6f, 1.0f };
    style.selectedVertexColor = { 0.7f, 0.8f, 0.9f, 1.0f };
    style.vertexBorderColor = { 0.05f, 0.06f, 0.07f, 1.0f };
    style.vertexRadiusPixels = 4.0f;
    style.hoveredVertexRadiusPixels = 5.0f;
    style.selectedVertexRadiusPixels = 6.0f;
    style.vertexBorderWidthPixels = 1.25f;

    editor::TopologyOverlayOptions options;
    options.style = style;

    editor::TopologyOverlayResult result;
    graphics::PointMarkerBatch batch =
        editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(
            scene,
            selection,
            options,
            &result);

    if (batch.size() != 4u ||
        result.visitedVertexCount != 4u ||
        result.normalVertexCount != 4u ||
        result.hoveredVertexCount != 0u ||
        result.selectedVertexCount != 0u) {
        return TestResult::fail("quad vertex overlay should produce four normal markers");
    }

    selection.mesh().set_hovered_vertex(handles.v1);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (result.hoveredVertexCount != 1u || result.normalVertexCount != 3u) {
        return TestResult::fail("vertex overlay should classify one hovered vertex");
    }

    selection.mesh().add_vertex(handles.v2);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (result.selectedVertexCount != 1u ||
        result.hoveredVertexCount != 1u ||
        result.normalVertexCount != 2u) {
        return TestResult::fail("vertex overlay should classify selected and hovered vertices");
    }

    selection.mesh().set_hovered_vertex(handles.v2);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (result.hoveredVertexCount != 0u || result.selectedVertexCount != 1u) {
        return TestResult::fail("selected vertex should take precedence over hover");
    }

    selection.mesh().add_vertex(handles.v0);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (result.selectedVertexCount != 2u) {
        return TestResult::fail("vertex overlay should support multi-selection");
    }

    bool sawSelectedStyle = false;
    for (const graphics::PointMarker& marker : batch.markers) {
        sawSelectedStyle = sawSelectedStyle ||
            (near(marker.radiusPixels, style.selectedVertexRadiusPixels) &&
                near(marker.borderWidthPixels, style.vertexBorderWidthPixels) &&
                near_color(marker.fillColor, style.selectedVertexColor) &&
                near_color(marker.borderColor, style.vertexBorderColor));
    }

    if (!sawSelectedStyle) {
        return TestResult::fail("vertex overlay should apply selected vertex style");
    }

    selection.mesh().add_vertex(locus::kernel::geometry::VertexHandle{ 500 });
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (result.invalidHandleCount != 1u || result.selectedVertexCount != 2u) {
        return TestResult::fail("invalid selected vertex handles should be diagnosed without emitting markers");
    }

    const editor::SceneNodeId parent = scene.create_empty("Parent");
    scene.find_node(parent)->transform().set_position({ 2.0f, 0.0f, 0.0f });
    scene.find_node(meshId)->transform().set_position({ 0.0f, 3.0f, 0.0f });
    scene.reparent(meshId, parent);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (batch.empty() ||
        !near_vec3(batch.markers.front().position, { 2.0f, 3.0f, 0.0f })) {
        return TestResult::fail("vertex overlay should apply parent and node transforms once");
    }

    selection.set_granularity(editor::SelectionGranularity::Edge);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("vertex overlay should return an empty batch outside vertex granularity");
    }

    selection.set_granularity(editor::SelectionGranularity::Face);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("vertex overlay should not leave stale markers in face granularity");
    }

    selection.set_granularity(editor::SelectionGranularity::Object);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, selection, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("vertex overlay should not leave stale markers in object granularity");
    }

    editor::SelectionState noActiveMesh;
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(scene, noActiveMesh, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("vertex overlay should return an empty batch without an active mesh");
    }

    editor::EditorScene emptyScene;
    const editor::SceneNodeId emptyMesh = emptyScene.create_mesh("Empty");
    editor::SelectionState emptySelection;
    emptySelection.set_scope(editor::SelectionScope::ActiveMesh);
    emptySelection.set_granularity(editor::SelectionGranularity::Vertex);
    emptySelection.mesh().set_active_mesh(emptyMesh);
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(emptyScene, emptySelection, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("vertex overlay should return an empty batch for an empty mesh");
    }

    emptyScene.find_mesh(emptyMesh)->mesh().add_vertex({ 3.0f, 4.0f, 5.0f });
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(emptyScene, emptySelection, options, &result);
    if (batch.size() != 1u || !near_vec3(batch.markers.front().position, { 3.0f, 4.0f, 5.0f })) {
        return TestResult::fail("vertex overlay should reflect topology additions");
    }

    emptyScene.find_mesh(emptyMesh)->mesh().clear();
    batch = editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(emptyScene, emptySelection, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("vertex overlay should reflect topology removals");
    }

    return TestResult::pass();
}

TestResult run_topology_overlay_face_surface_tests()
{
    editor::EditorScene scene;
    QuadHandles handles{};
    const editor::SceneNodeId meshId = make_quad_scene(scene, handles);
    const locus::kernel::geometry::FaceHandle quadFace =
        first_face(scene.find_mesh(meshId)->mesh());

    editor::SelectionState selection;
    selection.set_scope(editor::SelectionScope::ActiveMesh);
    selection.set_granularity(editor::SelectionGranularity::Face);
    selection.mesh().set_active_mesh(meshId);

    graphics::TopologyOverlayStyle style{};
    style.hoveredFaceColor = { 0.1f, 0.2f, 0.8f, 0.25f };
    style.selectedFaceColor = { 0.2f, 0.4f, 1.0f, 0.40f };

    editor::TopologyOverlayOptions options;
    options.style = style;

    editor::TopologyOverlayResult result;
    editor::TopologySurfaceOverlayBatches batches =
        editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
            scene,
            selection,
            options,
            &result);

    if (!batches.empty() ||
        result.visitedFaceCount != 1u ||
        result.hoveredFaceCount != 0u ||
        result.selectedFaceCount != 0u) {
        return TestResult::fail("face surface overlay should not emit normal faces");
    }

    selection.mesh().set_hovered_face(quadFace);
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    if (batches.hovered.triangle_count() != 2u ||
        !batches.selected.empty() ||
        result.hoveredFaceCount != 1u ||
        result.surfaceTriangleCount != 2u) {
        return TestResult::fail("hovered quad face should produce two surface overlay triangles");
    }

    if (!near_color(batches.hovered.vertices.front().color, style.hoveredFaceColor)) {
        return TestResult::fail("hovered face surface should use the hovered face style");
    }

    selection.mesh().add_face(quadFace);
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    if (!batches.hovered.empty() ||
        batches.selected.triangle_count() != 2u ||
        result.hoveredFaceCount != 0u ||
        result.selectedFaceCount != 1u) {
        return TestResult::fail("selected face should take precedence over hover");
    }

    if (!near_color(batches.selected.vertices.front().color, style.selectedFaceColor)) {
        return TestResult::fail("selected face surface should use the selected face style");
    }

    const editor::SceneNodeId triangleMesh = scene.create_mesh("Triangle");
    const locus::kernel::geometry::FaceHandle triangleFace =
        add_triangle(scene.find_mesh(triangleMesh)->mesh());
    selection.mesh().set_active_mesh(triangleMesh);
    selection.mesh().clear_components();
    selection.mesh().add_face(triangleFace);

    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    if (batches.selected.triangle_count() != 1u ||
        result.selectedFaceCount != 1u) {
        return TestResult::fail("selected triangular face should produce one overlay triangle");
    }

    const editor::SceneNodeId ngonMesh = scene.create_mesh("Ngon");
    const locus::kernel::geometry::FaceHandle ngonFace =
        add_ngon(scene.find_mesh(ngonMesh)->mesh());
    selection.mesh().set_active_mesh(ngonMesh);
    selection.mesh().clear_components();
    selection.mesh().add_face(ngonFace);

    const std::size_t faceCountBefore =
        locus::kernel::geometry::TopologyTraversal::faces(
            scene.find_mesh(ngonMesh)->mesh()).size();

    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    const std::size_t faceCountAfter =
        locus::kernel::geometry::TopologyTraversal::faces(
            scene.find_mesh(ngonMesh)->mesh()).size();

    if (batches.selected.triangle_count() != 3u ||
        faceCountBefore != faceCountAfter) {
        return TestResult::fail("selected n-gon should triangulate temporarily without altering LEM topology");
    }

    const editor::SceneNodeId multiMesh = scene.create_mesh("Multi");
    locus::kernel::geometry::LEM& multi = scene.find_mesh(multiMesh)->mesh();
    const locus::kernel::geometry::FaceHandle first = add_triangle(multi);
    const locus::kernel::geometry::FaceHandle second = add_ngon(multi);
    selection.mesh().set_active_mesh(multiMesh);
    selection.mesh().clear_components();
    selection.mesh().add_face(first);
    selection.mesh().add_face(second);

    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    if (batches.selected.triangle_count() != 4u ||
        result.selectedFaceCount != 2u) {
        return TestResult::fail("face surface overlay should batch multiple selected faces");
    }

    selection.mesh().add_face(locus::kernel::geometry::FaceHandle{ 500 });
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    if (result.invalidHandleCount != 1u ||
        batches.selected.triangle_count() != 4u) {
        return TestResult::fail("invalid selected face handles should be diagnosed without emitting triangles");
    }

    selection.mesh().clear_components();
    selection.mesh().set_hovered_face(locus::kernel::geometry::FaceHandle{ 501 });
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    if (!batches.empty() ||
        result.invalidHandleCount != 1u ||
        result.hoveredFaceCount != 0u ||
        result.selectedFaceCount != 0u) {
        return TestResult::fail("invalid hovered face handles should be diagnosed without emitting stale surfaces");
    }

    selection.mesh().set_active_mesh(meshId);
    selection.mesh().clear_components();
    selection.mesh().set_hovered_face(locus::kernel::geometry::FaceHandle{});
    selection.mesh().add_face(quadFace);

    const editor::SceneNodeId parent = scene.create_empty("Parent");
    scene.find_node(parent)->transform().set_position({ 2.0f, 0.0f, 0.0f });
    scene.find_node(meshId)->transform().set_position({ 0.0f, 3.0f, 0.0f });
    scene.reparent(meshId, parent);
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);

    if (batches.selected.empty() ||
        !near_vec3(batches.selected.vertices.front().position, { 0.0f, 0.0f, 0.0f }) ||
        !near_mat4_translation(batches.selected.modelMatrix, { 0.0f, 3.0f, 0.0f })) {
        return TestResult::fail("face surface overlay should keep local vertices and provide the render model transform");
    }

    selection.set_granularity(editor::SelectionGranularity::Edge);
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        selection,
        options,
        &result);
    if (!batches.empty() || result.has_geometry()) {
        return TestResult::fail("face surface overlay should not leave stale surfaces outside face granularity");
    }

    editor::SelectionState noActiveMesh;
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        scene,
        noActiveMesh,
        options,
        &result);
    if (!batches.empty() || result.has_geometry()) {
        return TestResult::fail("face surface overlay should return empty batches without an active mesh");
    }

    editor::EditorScene emptyScene;
    const editor::SceneNodeId emptyMesh = emptyScene.create_mesh("Empty");
    editor::SelectionState emptySelection;
    emptySelection.set_scope(editor::SelectionScope::ActiveMesh);
    emptySelection.set_granularity(editor::SelectionGranularity::Face);
    emptySelection.mesh().set_active_mesh(emptyMesh);
    batches = editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        emptyScene,
        emptySelection,
        options,
        &result);
    if (!batches.empty() || result.has_geometry()) {
        return TestResult::fail("face surface overlay should return empty batches for an empty mesh");
    }

    return TestResult::pass();
}

} // namespace locus::tests
