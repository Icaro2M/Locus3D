/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorRenderTestSuite.h"

#include "editor/render/TopologyOverlayAdapter.h"
#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionState.h"

#include <glm/vec3.hpp>

#include <cmath>

namespace {

constexpr float epsilon = 0.0001f;

struct QuadHandles {
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
        mesh.find_edge(v0, v1),
        mesh.find_edge(v1, v2),
        mesh.find_edge(v2, v3),
        mesh.find_edge(v3, v0)
    };
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

    editor::SelectionState noActiveMesh;
    batch = editor::TopologyOverlayAdapter::build_active_mesh_lines(scene, noActiveMesh, options, &result);
    if (!batch.empty() || result.has_geometry()) {
        return TestResult::fail("topology overlay should return an empty batch without an active mesh");
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

} // namespace locus::tests
