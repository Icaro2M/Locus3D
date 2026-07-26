/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string_view>
#include <thread>

namespace {

    using namespace locus::application;

    void print_result(bool condition, std::string_view message)
    {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

} // namespace

int main()
{
    std::cout
        << "=== Locus3D EditorViewport Visual Smoke Test ===\n";

    bool passed = true;
    ApplicationWindow window{};
    EditorViewport viewport{};

    const bool startsUninitialized =
        !window.initialized() && !viewport.initialized();
    print_result(
        startsUninitialized,
        "window and editor viewport start uninitialized");
    passed = startsUninitialized && passed;

    ApplicationConfig config{};
    config.title = "Locus3D - EditorViewport Smoke Test";
    config.initialWidth = 960;
    config.initialHeight = 540;
    config.enableVSync = true;

    const ApplicationResult<void> windowResult = window.initialize(config);
    if (!windowResult) {
        std::cerr << windowResult.error().message << '\n';
        return 1;
    }

    const bool windowInitialized =
        window.initialized()
        && window.framebuffer_width() > 0
        && window.framebuffer_height() > 0;
    print_result(
        windowInitialized,
        "application window and graphics context initialize");
    passed = windowInitialized && passed;

    DocumentManager documents{};
    DocumentSession& document = documents.create_document();

    const ApplicationResult<void> viewportResult = viewport.initialize(
        window.framebuffer_width(),
        window.framebuffer_height());

    if (!viewportResult) {
        std::cerr << viewportResult.error().message << '\n';
        window.shutdown();
        return 1;
    }

    const float expectedAspect =
        static_cast<float>(window.framebuffer_width())
        / static_cast<float>(window.framebuffer_height());
    const bool initialSizeIsSynchronized =
        viewport.initialized()
        && std::abs(viewport.aspect_ratio() - expectedAspect) < 0.001f;
    print_result(
        initialSizeIsSynchronized,
        "viewport initializes from framebuffer size and updates aspect ratio");
    passed = initialSizeIsSynchronized && passed;

    constexpr int SmokeFrameCount = 60;
    bool framesRendered = true;

    for (int frame = 0;
         frame < SmokeFrameCount && !window.should_close();
         ++frame) {
        window.process_events();
        viewport.resize(
            window.framebuffer_width(),
            window.framebuffer_height());

        const ApplicationResult<void> renderResult =
            viewport.render(document);

        if (!renderResult) {
            std::cerr << renderResult.error().message << '\n';
            framesRendered = false;
            break;
        }

        window.present();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    const locus::graphics::RenderStats& stats =
        viewport.renderer().stats();
    const bool visualFrameIsValid =
        framesRendered
        && stats.objectsSubmitted == 2
        && stats.objectsDrawn == 2
        && stats.drawCalls == 2;
    print_result(
        visualFrameIsValid,
        "empty document renders background, grid, and axes");
    passed = visualFrameIsValid && passed;

    const auto& viewportRect = viewport.viewport().state().rect;
    const bool resizeIsSynchronized =
        viewportRect.width
            == (window.framebuffer_width() > 0
                ? window.framebuffer_width()
                : 1)
        && viewportRect.height
            == (window.framebuffer_height() > 0
                ? window.framebuffer_height()
                : 1);
    print_result(
        resizeIsSynchronized,
        "framebuffer resize is processed before rendering");
    passed = resizeIsSynchronized && passed;

    window.request_close();
    window.process_events();
    const bool closeDetected = window.should_close();
    print_result(
        closeDetected,
        "window close request is detected");
    passed = closeDetected && passed;

    viewport.shutdown();
    viewport.shutdown();
    window.shutdown();
    window.shutdown();

    const bool shutdownIsIdempotent =
        !viewport.initialized() && !window.initialized();
    print_result(
        shutdownIsIdempotent,
        "viewport and window shutdown are idempotent");
    passed = shutdownIsIdempotent && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== EditorViewport visual smoke test passed ===\n";
        return 0;
    }

    std::cout
        << "=== EditorViewport visual smoke test failed ===\n";
    return 1;
}
