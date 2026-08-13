/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorSceneTestSuite.h"

#include "editor/Editor.h"
#include "editor/io/DocumentSceneIO.h"
#include "editor/io/Locus3DFormat.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/topology/TopologyBuilder.h"

#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <string>

namespace locus::tests {

namespace {

[[nodiscard]] bool near(float a, float b)
{
    return std::fabs(a - b) <= 1.0e-5f;
}

[[nodiscard]] bool near_vec3(const glm::vec3& a, const glm::vec3& b)
{
    return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z);
}

} // namespace

TestResult run_document_archive_tests()
{
    editor::Editor editor{};
    editor::EditorScene& scene = editor.scene();

    const editor::SceneNodeId parent = scene.create_empty("Parent");
    const editor::SceneNodeId meshId = scene.create_mesh("Mesh");
    if (!scene.reparent(meshId, parent)) {
        return TestResult::fail("Document test setup should create hierarchy");
    }

    editor::SceneNode* parentNode = scene.find_node(parent);
    editor::MeshNode* meshNode = scene.find_mesh(meshId);
    if (!parentNode || !meshNode) {
        return TestResult::fail("Document test setup should find nodes");
    }

    parentNode->metadata().visible = false;
    parentNode->metadata().locked = true;
    parentNode->metadata().selectable = false;
    parentNode->metadata().expanded = false;
    parentNode->transform().set_position({ 1.0f, 2.0f, 3.0f });
    parentNode->transform().set_rotation(
        glm::angleAxis(0.5f, glm::vec3{ 0.0f, 1.0f, 0.0f }));
    parentNode->transform().set_scale({ 2.0f, 3.0f, 4.0f });
    parentNode->pivot().custom = true;
    parentNode->pivot().offset = { 0.25f, 0.5f, 0.75f };

    kernel::geometry::TopologyBuilder::build_into(
        meshNode->mesh(),
        {
            { 0.0f, 0.0f, 0.0f },
            { 1.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 0.0f },
            { 0.5f, 1.5f, 0.0f },
            { 0.0f, 1.0f, 0.0f }
        },
        { { 0, 1, 2, 3, 4 } });

    meshNode->mesh().vertex(kernel::geometry::VertexHandle{ 0 }).hidden = true;
    meshNode->mesh().vertex(kernel::geometry::VertexHandle{ 0 }).selected = true;
    meshNode->mesh().vertex(kernel::geometry::VertexHandle{ 0 }).tag = 7u;
    meshNode->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).smooth = true;
    meshNode->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).crease = 0.75f;
    meshNode->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).hidden = true;
    meshNode->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).tag = 9u;
    meshNode->mesh().face(kernel::geometry::FaceHandle{ 0 }).hidden = true;
    meshNode->mesh().face(kernel::geometry::FaceHandle{ 0 }).tag = 11u;

    editor::DocumentArchiveResult archive =
        editor::capture_document_archive(scene);
    if (!archive.success) {
        return TestResult::fail("Document archive capture failed");
    }

    const std::string text =
        editor::serialize_locus_document(archive.archive);
    if (text.find("\"format\": \"Locus3D\"") == std::string::npos
        || text.find("\"version\": 1") == std::string::npos) {
        return TestResult::fail("Serialized document should include magic and version");
    }
    if (text.find("selected") != std::string::npos
        || text.find("normal") != std::string::npos) {
        return TestResult::fail("Document format should not persist selection or derived normals");
    }

    editor::DocumentArchiveResult parsed =
        editor::deserialize_locus_document(text);
    if (!parsed.success) {
        return TestResult::fail("Serialized document should parse");
    }

    editor::Editor loadedEditor{};
    editor::DocumentArchiveResult applied =
        editor::apply_document_archive(loadedEditor, parsed.archive);
    if (!applied.success) {
        return TestResult::fail("Parsed document should apply");
    }

    const auto roots = loadedEditor.scene().tree().roots();
    if (roots.size() != 1u) {
        return TestResult::fail("Loaded document should preserve one root");
    }

    const editor::SceneNode* loadedParent =
        loadedEditor.scene().find_node(roots.front());
    if (!loadedParent || loadedParent->children().size() != 1u) {
        return TestResult::fail("Loaded document should preserve hierarchy");
    }

    if (loadedParent->metadata().visible
        || !loadedParent->metadata().locked
        || loadedParent->metadata().selectable
        || loadedParent->metadata().expanded) {
        return TestResult::fail("Loaded document should preserve metadata");
    }

    if (!near_vec3(
            loadedParent->transform().position(),
            glm::vec3{ 1.0f, 2.0f, 3.0f })
        || !near_vec3(
            loadedParent->transform().scale(),
            glm::vec3{ 2.0f, 3.0f, 4.0f })
        || !loadedParent->pivot().custom
        || !near_vec3(
            loadedParent->pivot().offset,
            glm::vec3{ 0.25f, 0.5f, 0.75f })) {
        return TestResult::fail("Loaded document should preserve transform and pivot");
    }

    const editor::MeshNode* loadedMesh =
        loadedEditor.scene().find_mesh(loadedParent->children().front());
    if (!loadedMesh
        || loadedMesh->mesh().vertex_count() != 5u
        || loadedMesh->mesh().face_count() != 1u
        || loadedMesh->mesh().face_loops(kernel::geometry::FaceHandle{ 0 }).size()
        != 5u) {
        return TestResult::fail("Loaded document should preserve ngon topology");
    }

    if (!loadedMesh->mesh().vertex(kernel::geometry::VertexHandle{ 0 }).hidden
        || loadedMesh->mesh().vertex(kernel::geometry::VertexHandle{ 0 }).selected
        || loadedMesh->mesh().vertex(kernel::geometry::VertexHandle{ 0 }).tag != 7u
        || !loadedMesh->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).smooth
        || !near(
            loadedMesh->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).crease,
            0.75f)
        || loadedMesh->mesh().edge(kernel::geometry::EdgeHandle{ 0 }).tag != 9u
        || loadedMesh->mesh().face(kernel::geometry::FaceHandle{ 0 }).tag != 11u) {
        return TestResult::fail("Loaded document should preserve persistent mesh attributes only");
    }

    editor::DocumentArchiveResult badVersion =
        editor::deserialize_locus_document(
            "{ \"format\": \"Locus3D\", \"version\": 999, \"document\": { \"nodes\": [] } }");
    if (badVersion.success) {
        return TestResult::fail("Unsupported document versions should be rejected");
    }

    editor::DocumentArchiveResult badMagic =
        editor::deserialize_locus_document(
            "{ \"format\": \"Other\", \"version\": 1, \"document\": { \"nodes\": [] } }");
    if (badMagic.success) {
        return TestResult::fail("Invalid document magic should be rejected");
    }

    editor::DocumentArchiveResult corrupt =
        editor::deserialize_locus_document("{ not-json");
    if (corrupt.success) {
        return TestResult::fail("Corrupt JSON should be rejected");
    }

    const std::size_t beforeNodes = loadedEditor.scene().tree().size();
    editor::DocumentArchiveResult invalidParent =
        editor::deserialize_locus_document(
            "{ \"format\": \"Locus3D\", \"version\": 1, \"document\": { \"nodes\": ["
            "{ \"id\": 0, \"parent\": 42, \"type\": \"empty\","
            "\"metadata\": { \"name\": \"Bad\" },"
            "\"transform\": { \"position\": [0,0,0], \"rotation\": [1,0,0,0], \"scale\": [1,1,1] },"
            "\"pivot\": { \"offset\": [0,0,0], \"custom\": false } } ] } }");
    if (invalidParent.success) {
        return TestResult::fail("Invalid parent references should be rejected");
    }
    if (loadedEditor.scene().tree().size() != beforeNodes) {
        return TestResult::fail("Failed load should not mutate current editor");
    }

    editor::DocumentArchiveResult empty =
        editor::deserialize_locus_document(
            "{ \"format\": \"Locus3D\", \"version\": 1, \"document\": { \"nodes\": [] } }");
    if (!empty.success) {
        return TestResult::fail("Empty document should round-trip");
    }

    return TestResult::pass();
}

} // namespace locus::tests
