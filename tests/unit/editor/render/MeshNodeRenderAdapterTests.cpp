/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorRenderTestSuite.h"

#include "editor/render/MeshNodeRenderAdapter.h"
#include "editor/scene/MeshNode.h"

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

} // namespace

namespace locus::tests {

TestResult run_mesh_node_render_adapter_tests()
{
    editor::MeshNode node{ editor::SceneNodeId{ 42 }, "Renderable Mesh" };
    add_triangle(node.mesh());
    node.transform().set_position(glm::vec3{ 2.0f, 3.0f, 4.0f });
    node.transform().set_scale(glm::vec3{ 2.0f, 2.0f, 2.0f });

    editor::MeshNodeRenderOptions options;
    options.uploadOptions.color = graphics::ColorRGBA{ 0.1f, 0.2f, 0.3f, 1.0f };
    options.layer = graphics::RenderLayer::Overlay;
    options.selected = true;
    options.hovered = true;
    options.wireframe = true;

    editor::MeshNodeRenderResult uploadResult;
    const graphics::MeshUploadData upload =
        editor::MeshNodeRenderAdapter::build_upload_data(node, options, &uploadResult);

    if (upload.is_empty() ||
        upload.vertices.size() != 3u ||
        upload.indices.size() != 3u ||
        upload.vertices[0].color[0] != 0.1f ||
        uploadResult.nodeId != node.id() ||
        !uploadResult.hasUploadData ||
        uploadResult.skipped ||
        uploadResult.uploadResult.triangleCount != 1u ||
        uploadResult.message != "Mesh node upload data built successfully.") {
        return TestResult::fail("MeshNodeRenderAdapter should triangulate mesh nodes into upload data");
    }

    const graphics::RenderObject object =
        editor::MeshNodeRenderAdapter::build_render_object(node, nullptr, options);

    if (object.id != 42u ||
        object.name != "Renderable Mesh" ||
        object.mesh != nullptr ||
        object.layer != graphics::RenderLayer::Overlay ||
        !object.selected ||
        !object.hovered ||
        !object.wireframe ||
        object.transform.position != glm::vec3{ 2.0f, 3.0f, 4.0f } ||
        object.transform.scale != glm::vec3{ 2.0f, 2.0f, 2.0f } ||
        !object.visibility.visible ||
        !object.visibility.selectable ||
        object.visibility.castsShadow ||
        object.visibility.receivesShadow) {
        return TestResult::fail("MeshNodeRenderAdapter should copy node state into render objects");
    }

    node.metadata().visible = false;
    node.metadata().locked = true;
    const graphics::RenderObject hiddenObject =
        editor::MeshNodeRenderAdapter::build_render_object(node, nullptr);

    if (hiddenObject.visibility.visible ||
        hiddenObject.visibility.selectable) {
        return TestResult::fail("MeshNodeRenderAdapter should map hidden/locked metadata to visibility");
    }

    editor::MeshNode emptyNode{ editor::SceneNodeId{ 7 }, "Empty Mesh" };
    editor::MeshNodeRenderOptions emptyOptions;
    emptyOptions.reportEmptyMeshes = false;
    editor::MeshNodeRenderResult emptyResult;
    const graphics::MeshUploadData emptyUpload =
        editor::MeshNodeRenderAdapter::build_upload_data(emptyNode, emptyOptions, &emptyResult);

    if (!emptyUpload.is_empty() ||
        emptyResult.nodeId != emptyNode.id() ||
        emptyResult.hasUploadData ||
        !emptyResult.skipped ||
        !emptyResult.message.empty()) {
        return TestResult::fail("empty mesh nodes should be skipped and optionally suppress diagnostics");
    }

    const graphics::MeshRenderCacheKey key =
        editor::MeshNodeRenderAdapter::build_cache_key(node, 99u);
    if (key.ownerId != 42u ||
        key.revision != 99u ||
        !key.is_valid()) {
        return TestResult::fail("MeshNodeRenderAdapter should build stable cache keys from node id and revision");
    }

    return TestResult::pass();
}

} // namespace locus::tests
