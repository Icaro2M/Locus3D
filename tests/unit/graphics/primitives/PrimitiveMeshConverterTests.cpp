/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PrimitivesTestSuite.h"

#include "graphics/GraphicsPrimitives.h"

#include <cmath>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

} // namespace

namespace locus::tests {

TestResult run_primitive_mesh_converter_tests()
{
    using namespace graphics;

    PrimitiveMesh mesh;
    mesh.topology = PrimitiveTopology::Lines;
    mesh.indices = { 1, 0 };

    PrimitiveVertex first;
    first.position = glm::vec3{ 1.0f, 2.0f, 3.0f };
    first.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
    first.color = ColorRGBA{ 0.25f, 0.5f, 0.75f, 1.0f };

    PrimitiveVertex second;
    second.position = glm::vec3{ 4.0f, 5.0f, 6.0f };
    second.normal = glm::vec3{ 1.0f, 0.0f, 0.0f };
    second.color = ColorRGBA{ 1.0f, 0.75f, 0.5f, 0.25f };

    mesh.vertices = { first, second };

    const MeshUploadData upload = PrimitiveMeshConverter::to_upload_data(
        mesh,
        BufferUsage::Stream);

    if (upload.is_empty() ||
        !upload.has_indices() ||
        upload.topology != PrimitiveTopology::Lines ||
        upload.usage != BufferUsage::Stream ||
        upload.indices != mesh.indices ||
        upload.vertices.size() != 2) {
        return TestResult::fail("converter should preserve topology, usage, indices, and vertex count");
    }

    if (!near(upload.vertices[0].position[0], 1.0f) ||
        !near(upload.vertices[0].position[1], 2.0f) ||
        !near(upload.vertices[0].position[2], 3.0f) ||
        !near(upload.vertices[0].normal[0], 0.0f) ||
        !near(upload.vertices[0].normal[1], 1.0f) ||
        !near(upload.vertices[0].normal[2], 0.0f) ||
        !near(upload.vertices[0].color[0], 0.25f) ||
        !near(upload.vertices[0].color[1], 0.5f) ||
        !near(upload.vertices[0].color[2], 0.75f) ||
        !near(upload.vertices[0].color[3], 1.0f) ||
        !near(upload.vertices[1].position[0], 4.0f) ||
        !near(upload.vertices[1].color[3], 0.25f)) {
        return TestResult::fail("converter should copy primitive vertex attributes into MeshVertex arrays");
    }

    const MeshUploadData defaultUsage = PrimitiveMeshConverter::to_upload_data(mesh);
    if (defaultUsage.usage != BufferUsage::Dynamic) {
        return TestResult::fail("converter default usage should be Dynamic");
    }

    PrimitiveMesh empty;
    empty.topology = PrimitiveTopology::Points;
    const MeshUploadData emptyUpload = PrimitiveMeshConverter::to_upload_data(empty);
    if (!emptyUpload.is_empty() ||
        emptyUpload.has_indices() ||
        emptyUpload.topology != PrimitiveTopology::Points) {
        return TestResult::fail("converter should preserve topology for empty meshes without inventing data");
    }

    return TestResult::pass();
}

} // namespace locus::tests
