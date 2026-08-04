/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PrimitivesTestSuite.h"

#include "graphics/overlay/renderers/GizmoRenderer.h"

namespace {

[[nodiscard]] bool has_triangle_geometry(
    const locus::graphics::MeshUploadData& mesh)
{
    return mesh.topology == locus::graphics::PrimitiveTopology::Triangles &&
        !mesh.vertices.empty() &&
        !mesh.indices.empty() &&
        mesh.indices.size() % 3u == 0u;
}

[[nodiscard]] float first_alpha(
    const locus::graphics::MeshUploadData& mesh)
{
    return mesh.vertices.empty() ? 0.0f : mesh.vertices.front().color[3];
}

} // namespace

namespace locus::tests {

TestResult run_gizmo_renderer_geometry_tests()
{
    using namespace graphics;

    GizmoRendererConfig config{};
    config.radialSegments = 24;
    config.ringMajorSegments = 64;
    config.ringMinorSegments = 8;

    const struct Entry {
        GizmoVisualMode mode;
        GizmoVisualHandle handle;
    } entries[] = {
        { GizmoVisualMode::Translate, GizmoVisualHandle::X },
        { GizmoVisualMode::Translate, GizmoVisualHandle::Y },
        { GizmoVisualMode::Translate, GizmoVisualHandle::Z },
        { GizmoVisualMode::Translate, GizmoVisualHandle::XY },
        { GizmoVisualMode::Translate, GizmoVisualHandle::XZ },
        { GizmoVisualMode::Translate, GizmoVisualHandle::YZ },
        { GizmoVisualMode::Translate, GizmoVisualHandle::XYZ },
        { GizmoVisualMode::Rotate, GizmoVisualHandle::X },
        { GizmoVisualMode::Rotate, GizmoVisualHandle::Y },
        { GizmoVisualMode::Rotate, GizmoVisualHandle::Z },
        { GizmoVisualMode::Rotate, GizmoVisualHandle::View },
        { GizmoVisualMode::Scale, GizmoVisualHandle::X },
        { GizmoVisualMode::Scale, GizmoVisualHandle::Y },
        { GizmoVisualMode::Scale, GizmoVisualHandle::Z },
        { GizmoVisualMode::Scale, GizmoVisualHandle::XY },
        { GizmoVisualMode::Scale, GizmoVisualHandle::XZ },
        { GizmoVisualMode::Scale, GizmoVisualHandle::YZ },
        { GizmoVisualMode::Scale, GizmoVisualHandle::XYZ },
    };

    for (const Entry& entry : entries) {
        const MeshUploadData mesh =
            GizmoRenderer::build_handle_mesh_data(
                entry.mode,
                entry.handle,
                config);

        if (!has_triangle_geometry(mesh)) {
            return TestResult::fail("each gizmo handle should build a single indexed triangle mesh");
        }
    }

    const MeshUploadData translateX =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Translate,
            GizmoVisualHandle::X,
            config);
    const MeshUploadData scaleX =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Scale,
            GizmoVisualHandle::X,
            config);
    if (translateX.vertices.size() == scaleX.vertices.size()) {
        return TestResult::fail("translate axes should use cone tips and scale axes should use cube tips");
    }

    const MeshUploadData rotateZ =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Rotate,
            GizmoVisualHandle::Z,
            config);
    if (rotateZ.vertices.size() !=
        static_cast<std::size_t>(config.ringMajorSegments * config.ringMinorSegments)) {
        return TestResult::fail("rotation rings should use the configured torus segment counts");
    }

    const MeshUploadData normalPlane =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Translate,
            GizmoVisualHandle::XY,
            config,
            GizmoVisualRole::Normal);
    const MeshUploadData hoverPlane =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Translate,
            GizmoVisualHandle::XY,
            config,
            GizmoVisualRole::Hovered);
    const MeshUploadData activePlane =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Translate,
            GizmoVisualHandle::XY,
            config,
            GizmoVisualRole::Active);
    const MeshUploadData disabledPlane =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Translate,
            GizmoVisualHandle::XY,
            config,
            GizmoVisualRole::Disabled);

    if (!(first_alpha(activePlane) >= first_alpha(hoverPlane) &&
        first_alpha(hoverPlane) > first_alpha(normalPlane) &&
        first_alpha(normalPlane) > first_alpha(disabledPlane))) {
        return TestResult::fail("plane roles should increase alpha for hover/active and reduce disabled contrast");
    }

    const MeshUploadData translateCenter =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Translate,
            GizmoVisualHandle::XYZ,
            config);
    const MeshUploadData scaleCenter =
        GizmoRenderer::build_handle_mesh_data(
            GizmoVisualMode::Scale,
            GizmoVisualHandle::XYZ,
            config);
    if (translateCenter.vertices.size() == scaleCenter.vertices.size()) {
        return TestResult::fail("translate and scale centers should use visually distinct meshes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
