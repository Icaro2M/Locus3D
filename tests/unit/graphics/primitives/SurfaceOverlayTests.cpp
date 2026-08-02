/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PrimitivesTestSuite.h"

#include "graphics/overlay/renderers/SurfaceOverlayRenderer.h"
#include "graphics/primitives/SurfaceOverlay.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>

#include <limits>

namespace locus::tests {

TestResult run_surface_overlay_tests()
{
    graphics::SurfaceOverlayBatch batch{};
    if (!batch.empty() ||
        batch.vertex_count() != 0u ||
        batch.index_count() != 0u ||
        batch.triangle_count() != 0u) {
        return TestResult::fail("empty surface overlay batch should report no geometry");
    }

    batch.vertices.push_back({ { 0.0f, 0.0f, 0.0f }, { 0.1f, 0.2f, 0.3f, 0.4f } });
    batch.vertices.push_back({ { 1.0f, 0.0f, 0.0f }, { 0.1f, 0.2f, 0.3f, 0.4f } });
    batch.vertices.push_back({ { 0.0f, 1.0f, 0.0f }, { 0.1f, 0.2f, 0.3f, 0.4f } });
    batch.indices = { 0u, 1u, 2u };

    if (batch.empty() ||
        batch.vertex_count() != 3u ||
        batch.index_count() != 3u ||
        batch.triangle_count() != 1u) {
        return TestResult::fail("surface overlay batch should preserve vertices and indices");
    }

    const glm::vec4 defaultOrigin =
        batch.modelMatrix * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
    if (defaultOrigin.x != 0.0f ||
        defaultOrigin.y != 0.0f ||
        defaultOrigin.z != 0.0f ||
        defaultOrigin.w != 1.0f) {
        return TestResult::fail("surface overlay batch should default to an identity model matrix");
    }

    batch.modelMatrix =
        glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 2.0f, 3.0f, 4.0f });
    const glm::vec4 translatedOrigin =
        batch.modelMatrix * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
    if (translatedOrigin.x != 2.0f ||
        translatedOrigin.y != 3.0f ||
        translatedOrigin.z != 4.0f ||
        translatedOrigin.w != 1.0f) {
        return TestResult::fail("surface overlay batch should carry the mesh model transform");
    }

    if (!graphics::is_drawable(batch.vertices.front()) ||
        !graphics::is_drawable_triangle(batch, 0u)) {
        return TestResult::fail("valid surface overlay triangle should be drawable");
    }

    if (batch.vertices.front().color.r != 0.1f ||
        batch.vertices.front().color.g != 0.2f ||
        batch.vertices.front().color.b != 0.3f ||
        batch.vertices.front().color.a != 0.4f) {
        return TestResult::fail("surface overlay vertex should preserve color and alpha");
    }

    graphics::SurfaceOverlayBatch invalidIndex = batch;
    invalidIndex.indices = { 0u, 1u, 5u };
    if (graphics::is_drawable_triangle(invalidIndex, 0u)) {
        return TestResult::fail("surface overlay triangle should reject invalid indices");
    }

    graphics::SurfaceOverlayBatch degenerate = batch;
    degenerate.vertices[2].position = degenerate.vertices[1].position;
    if (graphics::is_drawable_triangle(degenerate, 0u)) {
        return TestResult::fail("surface overlay triangle should reject degenerate triangles");
    }

    graphics::SurfaceOverlayVertex transparent = batch.vertices.front();
    transparent.color.a = 0.0f;
    if (graphics::is_drawable(transparent)) {
        return TestResult::fail("surface overlay vertex should reject transparent vertices");
    }

    graphics::SurfaceOverlayVertex nonFinite = batch.vertices.front();
    nonFinite.position.x = std::numeric_limits<float>::infinity();
    if (graphics::is_drawable(nonFinite)) {
        return TestResult::fail("surface overlay vertex should reject non-finite positions");
    }

    graphics::SurfaceOverlayRendererConfig config{};
    if (!config.depthTest ||
        config.depthWrite ||
        config.depthFunc != graphics::DepthFunc::LessEqual ||
        !config.blend ||
        config.cullFace) {
        return TestResult::fail("surface overlay visible-only policy should use scene depth, alpha blend, and no culling by default");
    }

    return TestResult::pass();
}

} // namespace locus::tests
