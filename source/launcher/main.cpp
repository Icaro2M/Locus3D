/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"

#include <iostream>
#include <string_view>

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
        << "=== Locus3D ApplicationRuntime Smoke Test ===\n";

    bool passed = true;
    ApplicationRuntime runtime{};

    const bool startsUninitialized =
        !runtime.initialized()
        && runtime.state().phase
            == ApplicationPhase::Uninitialized
        && !runtime.window().initialized();
    print_result(
        startsUninitialized,
        "runtime and window start uninitialized");
    passed = startsUninitialized && passed;

    ApplicationConfig config{};
    config.title = "Locus3D ApplicationRuntime Smoke Test";
    config.initialWidth = 960;
    config.initialHeight = 540;
    config.decorated = true;
    config.startMaximized = false;
    config.enableVSync = true;
    config.maximumFrameDeltaSeconds = 0.1;

    const ApplicationResult<void> initializeResult =
        runtime.initialize(config);
    const bool initialized =
        initializeResult.ok()
        && runtime.initialized()
        && runtime.window().initialized()
        && runtime.state().phase == ApplicationPhase::Running;
    print_result(
        initialized,
        "initialize creates the application window and starts the runtime");
    passed = initialized && passed;

    if (!initializeResult) {
        std::cerr
            << "Initialization error: "
            << initializeResult.error().message
            << '\n';
        runtime.shutdown();
        return 1;
    }

    const bool configurationIsPreserved =
        runtime.configuration().title == config.title
        && runtime.configuration().initialWidth
            == config.initialWidth
        && runtime.configuration().initialHeight
            == config.initialHeight
        && runtime.window().configuration().enableVSync
            == config.enableVSync;
    print_result(
        configurationIsPreserved,
        "runtime forwards and preserves application configuration");
    passed = configurationIsPreserved && passed;

    ApplicationResult<FrameContext> firstFrame =
        runtime.run_frame();
    ApplicationResult<FrameContext> secondFrame =
        runtime.run_frame();
    ApplicationResult<FrameContext> thirdFrame =
        runtime.run_frame();

    const bool framesExecuted =
        firstFrame.ok()
        && secondFrame.ok()
        && thirdFrame.ok();
    const bool frameClockAdvanced =
        framesExecuted
        && firstFrame.value().frameIndex == 0
        && firstFrame.value().firstFrame
        && secondFrame.value().frameIndex == 1
        && !secondFrame.value().firstFrame
        && thirdFrame.value().frameIndex == 2
        && runtime.state().frameIndex == 3;
    const bool deltasAreBounded =
        framesExecuted
        && firstFrame.value().deltaSeconds <= 0.1
        && secondFrame.value().deltaSeconds <= 0.1
        && thirdFrame.value().deltaSeconds <= 0.1;
    print_result(
        framesExecuted,
        "main-loop iterations process events and present frames");
    print_result(
        frameClockAdvanced,
        "FrameClock advances frame contexts and runtime state");
    print_result(
        deltasAreBounded,
        "runtime applies the configured maximum frame delta");
    passed =
        framesExecuted
        && frameClockAdvanced
        && deltasAreBounded
        && passed;

    runtime.request_quit(0);
    const bool quitWasRequested =
        runtime.state().quitRequested
        && runtime.window().should_close();
    print_result(
        quitWasRequested,
        "quit request reaches runtime and window state");
    passed = quitWasRequested && passed;

    const ApplicationResult<int> runResult = runtime.run();
    const bool runCompleted =
        runResult.ok()
        && runResult.value() == 0;
    const bool shutdownCompleted =
        !runtime.initialized()
        && !runtime.window().initialized()
        && runtime.state().phase == ApplicationPhase::Stopped
        && runtime.state().frameIndex == 3;
    print_result(
        runCompleted,
        "run exits with the configured process code");
    print_result(
        shutdownCompleted,
        "run shuts down the window and preserves final state");
    passed = runCompleted && shutdownCompleted && passed;

    runtime.shutdown();
    runtime.shutdown();
    const bool shutdownIsIdempotent =
        runtime.state().phase == ApplicationPhase::Stopped
        && !runtime.window().initialized();
    print_result(
        shutdownIsIdempotent,
        "runtime shutdown is idempotent");
    passed = shutdownIsIdempotent && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All ApplicationRuntime smoke tests passed ===\n";
        return 0;
    }

    std::cout
        << "=== ApplicationRuntime smoke test failed ===\n";
    return 1;
}
