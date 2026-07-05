/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorRenderTestSuite.h"

#include "editor/render/SceneRenderAdapter.h"
#include "editor/scene/EditorScene.h"

#include <glm/vec3.hpp>

namespace {

void add_triangle(locus::kernel::geometry::LEM& mesh)
{
    using namespace locus::kernel::geometry;

    const VertexHandle v0 = mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    mesh.add_face({ v0, v1, v2 });
}

[[nodiscard]] bool contains_object(
    const locus::graphics::RenderScene& scene,
    locus::editor::SceneNodeId id)
{
    for (const locus::graphics::RenderObject& object : scene.objects()) {
        if (object.id == id.value) {
            return true;
        }
    }

    return false;
}

} // namespace

namespace locus::tests {

TestResult run_scene_render_adapter_tests()
{
    editor::EditorScene scene;
    const editor::SceneNodeId empty = scene.create_empty("Empty");
    const editor::SceneNodeId visibleMesh = scene.create_mesh("Visible Mesh");
    const editor::SceneNodeId hiddenMesh = scene.create_mesh("Hidden Mesh");

    add_triangle(scene.find_mesh(visibleMesh)->mesh());
    add_triangle(scene.find_mesh(hiddenMesh)->mesh());
    scene.find_node(hiddenMesh)->metadata().visible = false;

    editor::SceneRenderOptions options;
    options.allowNullGpuMeshes = true;
    options.includeHiddenNodes = true;
    options.meshOptions.layer = graphics::RenderLayer::Debug;

    editor::SceneRenderResult result;
    const graphics::RenderScene renderScene =
        editor::SceneRenderAdapter::build_render_scene(scene, {}, options, &result);

    if (renderScene.object_count() != 2u ||
        !contains_object(renderScene, visibleMesh) ||
        !contains_object(renderScene, hiddenMesh) ||
        result.visitedNodeCount != 3u ||
        result.meshNodeCount != 2u ||
        result.objectCount != 2u ||
        result.skippedNodeCount != 1u ||
        result.failedNodeCount != 0u ||
        result.has_failures() ||
        result.nodes.size() != 3u) {
        return TestResult::fail("SceneRenderAdapter should emit mesh nodes and skip non-mesh nodes");
    }

    if (renderScene.objects()[0].mesh != nullptr ||
        renderScene.objects()[0].layer != graphics::RenderLayer::Debug) {
        return TestResult::fail("SceneRenderAdapter should allow null GPU meshes and forward mesh options");
    }

    editor::SceneRenderOptions skipHiddenOptions;
    skipHiddenOptions.allowNullGpuMeshes = true;
    skipHiddenOptions.includeHiddenNodes = false;
    editor::SceneRenderResult skipHiddenResult;
    const graphics::RenderScene visibleOnlyScene =
        editor::SceneRenderAdapter::build_render_scene(
            scene,
            {},
            skipHiddenOptions,
            &skipHiddenResult);

    if (visibleOnlyScene.object_count() != 1u ||
        !contains_object(visibleOnlyScene, visibleMesh) ||
        contains_object(visibleOnlyScene, hiddenMesh) ||
        skipHiddenResult.skippedNodeCount != 2u ||
        skipHiddenResult.objectCount != 1u) {
        return TestResult::fail("SceneRenderAdapter should skip hidden meshes when requested");
    }

    editor::SceneRenderOptions requireGpuOptions;
    requireGpuOptions.allowNullGpuMeshes = false;
    editor::SceneRenderResult requireGpuResult;
    const graphics::RenderScene noGpuScene =
        editor::SceneRenderAdapter::build_render_scene(
            scene,
            {},
            requireGpuOptions,
            &requireGpuResult);

    if (!noGpuScene.empty() ||
        requireGpuResult.objectCount != 0u ||
        requireGpuResult.skippedNodeCount != 3u ||
        requireGpuResult.failedNodeCount != 0u) {
        return TestResult::fail("SceneRenderAdapter should skip mesh nodes without GPU meshes when nulls are disallowed");
    }

    (void)empty;
    return TestResult::pass();
}

} // namespace locus::tests
