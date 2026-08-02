/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PrimitivesTestSuite.h"

#include "graphics/overlay/renderers/PointMarkerRenderer.h"
#include "graphics/primitives/PointMarker.h"

#include <limits>

namespace locus::tests {

TestResult run_point_marker_tests()
{
    graphics::PointMarker marker{};
    marker.position = { 1.0f, 2.0f, 3.0f };
    marker.fillColor = { 0.2f, 0.4f, 0.6f, 0.8f };
    marker.borderColor = { 0.1f, 0.3f, 0.5f, 0.7f };
    marker.radiusPixels = 5.0f;
    marker.borderWidthPixels = 1.5f;

    if (!graphics::is_drawable(marker)) {
        return TestResult::fail("valid point marker should be drawable");
    }

    if (marker.position.x != 1.0f ||
        marker.position.y != 2.0f ||
        marker.position.z != 3.0f) {
        return TestResult::fail("point marker should preserve position");
    }

    if (marker.fillColor.r != 0.2f ||
        marker.fillColor.g != 0.4f ||
        marker.fillColor.b != 0.6f ||
        marker.fillColor.a != 0.8f ||
        marker.borderColor.r != 0.1f ||
        marker.borderColor.g != 0.3f ||
        marker.borderColor.b != 0.5f ||
        marker.borderColor.a != 0.7f) {
        return TestResult::fail("point marker should preserve colors");
    }

    graphics::PointMarker zeroRadius = marker;
    zeroRadius.radiusPixels = 0.0f;
    if (graphics::is_drawable(zeroRadius)) {
        return TestResult::fail("zero-radius point marker should be rejected");
    }

    graphics::PointMarker negativeRadius = marker;
    negativeRadius.radiusPixels = -1.0f;
    if (graphics::is_drawable(negativeRadius)) {
        return TestResult::fail("negative-radius point marker should be rejected");
    }

    graphics::PointMarker zeroBorder = marker;
    zeroBorder.borderWidthPixels = 0.0f;
    if (!graphics::is_drawable(zeroBorder)) {
        return TestResult::fail("zero-border point marker should be accepted");
    }

    graphics::PointMarker negativeBorder = marker;
    negativeBorder.borderWidthPixels = -1.0f;
    if (graphics::is_drawable(negativeBorder)) {
        return TestResult::fail("negative-border point marker should be rejected");
    }

    graphics::PointMarker borderLargerThanRadius = marker;
    borderLargerThanRadius.borderWidthPixels = marker.radiusPixels + 2.0f;
    if (!graphics::is_drawable(borderLargerThanRadius)) {
        return TestResult::fail("border larger than radius should stay drawable and be renderer-sanitized");
    }

    graphics::PointMarker nonFinite = marker;
    nonFinite.position.x = std::numeric_limits<float>::infinity();
    if (graphics::is_drawable(nonFinite)) {
        return TestResult::fail("non-finite point marker should be rejected");
    }

    graphics::PointMarkerBatch batch;
    if (!batch.empty() || batch.size() != 0u) {
        return TestResult::fail("empty point marker batch should report empty");
    }

    batch.markers.push_back(marker);
    if (batch.empty() || batch.size() != 1u) {
        return TestResult::fail("point marker batch should report stored marker count");
    }

    graphics::PointMarkerRendererConfig config{};
    if (!config.depthTest ||
        config.depthWrite ||
        config.depthFunc != graphics::DepthFunc::LessEqual ||
        !config.blend) {
        return TestResult::fail("point marker visible-only policy should use the scene depth buffer without writes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
