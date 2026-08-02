/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PrimitivesTestSuite.h"

#include "graphics/overlay/renderers/ScreenSpaceLineRenderer.h"
#include "graphics/primitives/ScreenSpaceLine.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <limits>

namespace {

[[nodiscard]] float ndc_depth_at_distance(float distance)
{
    const glm::mat4 projection = glm::perspective(
        0.78539816339f,
        16.0f / 9.0f,
        0.01f,
        1000.0f);
    const glm::vec4 clip =
        projection * glm::vec4{ 0.0f, 0.0f, -distance, 1.0f };
    return clip.z / clip.w;
}

} // namespace

namespace locus::tests {

TestResult run_screen_space_line_tests()
{
    graphics::ScreenSpaceLine line{};
    line.start = { 0.0f, 0.0f, 0.0f };
    line.end = { 1.0f, 0.0f, 0.0f };
    line.color = { 0.2f, 0.4f, 0.6f, 0.8f };
    line.widthPixels = 2.0f;

    if (!graphics::is_drawable(line)) {
        return TestResult::fail("valid screen-space line should be drawable");
    }

    if (line.color.r != 0.2f ||
        line.color.g != 0.4f ||
        line.color.b != 0.6f ||
        line.color.a != 0.8f) {
        return TestResult::fail("screen-space line should preserve colors");
    }

    graphics::ScreenSpaceLine zeroWidth = line;
    zeroWidth.widthPixels = 0.0f;
    if (graphics::is_drawable(zeroWidth)) {
        return TestResult::fail("zero-width screen-space line should be rejected");
    }

    graphics::ScreenSpaceLine negativeWidth = line;
    negativeWidth.widthPixels = -1.0f;
    if (graphics::is_drawable(negativeWidth)) {
        return TestResult::fail("negative-width screen-space line should be rejected");
    }

    graphics::ScreenSpaceLine degenerate = line;
    degenerate.end = degenerate.start;
    if (graphics::is_drawable(degenerate)) {
        return TestResult::fail("degenerate screen-space line should be rejected");
    }

    graphics::ScreenSpaceLine nonFinite = line;
    nonFinite.start.x = std::numeric_limits<float>::infinity();
    if (graphics::is_drawable(nonFinite)) {
        return TestResult::fail("non-finite screen-space line should be rejected");
    }

    graphics::ScreenSpaceLineBatch batch;
    if (!batch.empty() || batch.size() != 0u) {
        return TestResult::fail("empty screen-space line batch should report empty");
    }

    batch.lines.push_back(line);
    if (batch.empty() || batch.size() != 1u) {
        return TestResult::fail("screen-space line batch should report stored line count");
    }

    graphics::ScreenSpaceLineRendererConfig config{};
    if (!config.depthTest ||
        config.depthWrite ||
        config.depthFunc != graphics::DepthFunc::LessEqual ||
        !config.blend) {
        return TestResult::fail("screen-space line visible-only policy should use the scene depth buffer without writes");
    }

    const float closeDepthGap =
        ndc_depth_at_distance(3.0f) - ndc_depth_at_distance(2.0f);
    const float farDepthGap =
        ndc_depth_at_distance(501.0f) - ndc_depth_at_distance(500.0f);
    constexpr float oldFixedNdcBias = 0.0006f;

    if (!(closeDepthGap > oldFixedNdcBias &&
        farDepthGap < oldFixedNdcBias)) {
        return TestResult::fail("fixed NDC depth bias should be proven distance-dependent for the production projection");
    }

    return TestResult::pass();
}

} // namespace locus::tests
