/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SnappingTestSuite.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/snapping/EdgeSnapProvider.h"
#include "editor/snapping/FaceSnapProvider.h"
#include "editor/snapping/VertexSnapProvider.h"

#include <cmath>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] locus::editor::MeshNode* add_snap_mesh(
    locus::editor::EditorScene& scene,
    locus::editor::SceneNodeId& nodeId)
{
    using namespace locus::kernel::geometry;

    nodeId = scene.create_mesh("Snap Mesh");
    locus::editor::MeshNode* node = scene.find_mesh(nodeId);
    if (!node) {
        return nullptr;
    }

    LEM& mesh = node->mesh();
    const VertexHandle v0 = mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 2.0f, 1.0f, 0.0f });
    const VertexHandle v3 = mesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    mesh.add_face({ v0, v1, v2, v3 });

    node->transform().set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });
    return node;
}

} // namespace

namespace locus::tests {

TestResult run_scene_snap_provider_tests()
{
    using namespace editor;

    SnapSettings settings;
    settings.set_modes(SnapMode::Vertex | SnapMode::Edge | SnapMode::Face);

    SnapContext noSceneContext;
    VertexSnapProvider vertexProvider;
    EdgeSnapProvider edgeProvider;
    FaceSnapProvider faceProvider;
    if (vertexProvider.is_enabled(settings, noSceneContext) ||
        edgeProvider.is_enabled(settings, noSceneContext) ||
        faceProvider.is_enabled(settings, noSceneContext) ||
        vertexProvider.snap(settings, noSceneContext).is_valid() ||
        edgeProvider.snap(settings, noSceneContext).is_valid() ||
        faceProvider.snap(settings, noSceneContext).is_valid()) {
        return TestResult::fail("scene snap providers should require a scene");
    }

    EditorScene scene;
    SceneNodeId nodeId;
    MeshNode* node = add_snap_mesh(scene, nodeId);
    if (!node) {
        return TestResult::fail("test scene should create a mesh node");
    }

    SnapContext context;
    context.scene = &scene;
    context.originalPosition = glm::vec3{ 0.0f, 0.0f, 0.0f };

    context.candidatePosition = glm::vec3{ 3.1f, 0.1f, 0.0f };
    const SnapResult vertexResult = vertexProvider.snap(settings, context);
    if (vertexProvider.mode() != SnapMode::Vertex ||
        !vertexProvider.is_enabled(settings, context) ||
        !vertexResult.is_valid() ||
        vertexResult.target.type != SnapTargetType::Vertex ||
        vertexResult.target.node != nodeId ||
        vertexResult.target.component != 1u ||
        !near_vec3(vertexResult.snappedPosition, glm::vec3{ 3.0f, 0.0f, 0.0f })) {
        return TestResult::fail("VertexSnapProvider should snap to the nearest visible mesh vertex in world space");
    }

    node->mesh().vertex(kernel::geometry::VertexHandle{ 1 }).hidden = true;
    const SnapResult hiddenVertexResult = vertexProvider.snap(settings, context);
    if (!hiddenVertexResult.is_valid() ||
        hiddenVertexResult.target.component == 1u) {
        return TestResult::fail("VertexSnapProvider should skip hidden vertices");
    }
    node->mesh().vertex(kernel::geometry::VertexHandle{ 1 }).hidden = false;

    context.candidatePosition = glm::vec3{ 2.0f, 0.25f, 0.0f };
    const SnapResult edgeResult = edgeProvider.snap(settings, context);
    if (edgeProvider.mode() != SnapMode::Edge ||
        !edgeProvider.is_enabled(settings, context) ||
        !edgeResult.is_valid() ||
        edgeResult.target.type != SnapTargetType::Edge ||
        edgeResult.target.node != nodeId ||
        !near_vec3(edgeResult.snappedPosition, glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
        !near_vec3(edgeResult.target.normal, glm::vec3{ 1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("EdgeSnapProvider should snap to the nearest point on a visible mesh edge");
    }

    node->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).hidden = true;
    const SnapResult hiddenEdgeResult = edgeProvider.snap(settings, context);
    if (!hiddenEdgeResult.is_valid() ||
        hiddenEdgeResult.target.component == 0u) {
        return TestResult::fail("EdgeSnapProvider should skip hidden edges");
    }
    node->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).hidden = false;

    context.candidatePosition = glm::vec3{ 1.5f, 0.5f, 0.4f };
    const SnapResult faceResult = faceProvider.snap(settings, context);
    if (faceProvider.mode() != SnapMode::Face ||
        !faceProvider.is_enabled(settings, context) ||
        !faceResult.is_valid() ||
        faceResult.target.type != SnapTargetType::Face ||
        faceResult.target.node != nodeId ||
        faceResult.target.component != 0u ||
        !near_vec3(faceResult.snappedPosition, glm::vec3{ 1.5f, 0.5f, 0.0f }) ||
        !near_vec3(faceResult.target.normal, glm::vec3{ 0.0f, 0.0f, 1.0f })) {
        return TestResult::fail("FaceSnapProvider should project candidates onto visible mesh faces");
    }

    context.candidatePosition = glm::vec3{ 4.0f, 4.0f, 0.4f };
    if (faceProvider.snap(settings, context).is_valid()) {
        return TestResult::fail("FaceSnapProvider should reject projected points outside the face polygon");
    }

    node->metadata().visible = false;
    context.candidatePosition = glm::vec3{ 3.1f, 0.1f, 0.0f };
    if (vertexProvider.snap(settings, context).is_valid() ||
        edgeProvider.snap(settings, context).is_valid() ||
        faceProvider.snap(settings, context).is_valid()) {
        return TestResult::fail("scene snap providers should skip invisible nodes");
    }

    node->metadata().visible = true;
    node->metadata().selectable = false;
    if (vertexProvider.snap(settings, context).is_valid() ||
        edgeProvider.snap(settings, context).is_valid() ||
        faceProvider.snap(settings, context).is_valid()) {
        return TestResult::fail("scene snap providers should skip unselectable nodes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
