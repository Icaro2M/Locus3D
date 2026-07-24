/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string_view>

namespace {

    using namespace locus::application;

    constexpr double TimeEpsilon = 0.000001;

    void print_result(bool condition, std::string_view message)
    {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

    [[nodiscard]] bool approximately_equal(
        double left,
        double right) noexcept
    {
        return std::abs(left - right) <= TimeEpsilon;
    }

    [[nodiscard]] bool test_application_config()
    {
        std::cout << "\n=== ApplicationConfig ===\n";

        const ApplicationConfig config{};

        const bool titleIsValid = config.title == "Locus3D";
        const bool dimensionsAreValid =
            config.initialWidth == 1280
            && config.initialHeight == 720;
        const bool windowOptionsAreValid =
            !config.startMaximized
            && config.decorated
            && config.enableVSync;
        const bool maximumDeltaIsValid =
            approximately_equal(
                config.maximumFrameDeltaSeconds,
                0.25);

        print_result(titleIsValid, "default title is Locus3D");
        print_result(
            dimensionsAreValid,
            "default window dimensions are 1280x720");
        print_result(
            windowOptionsAreValid,
            "default window options are consistent");
        print_result(
            maximumDeltaIsValid,
            "default maximum frame delta is 0.25 seconds");

        return titleIsValid
            && dimensionsAreValid
            && windowOptionsAreValid
            && maximumDeltaIsValid;
    }

    [[nodiscard]] bool test_application_error_and_result()
    {
        std::cout << "\n=== ApplicationError and ApplicationResult ===\n";

        const ApplicationError noError =
            ApplicationError::none();
        const ApplicationError runtimeError =
            ApplicationError::make(
                ApplicationErrorCode::RuntimeFailure,
                "runtime smoke-test failure");

        const ApplicationResult<int> valueResult{ 42 };
        const ApplicationResult<int> failedValueResult{
            runtimeError
        };
        const ApplicationResult<void> voidResult{};
        const ApplicationResult<void> failedVoidResult{
            runtimeError
        };

        const bool emptyErrorIsValid =
            !noError.has_error()
            && !static_cast<bool>(noError);
        const bool populatedErrorIsValid =
            runtimeError.has_error()
            && runtimeError.code
                == ApplicationErrorCode::RuntimeFailure
            && runtimeError.message
                == "runtime smoke-test failure";
        const bool valueSuccessIsValid =
            valueResult.ok()
            && !valueResult.failed()
            && valueResult.value() == 42
            && !valueResult.error().has_error();
        const bool valueFailureIsValid =
            failedValueResult.failed()
            && !failedValueResult.ok()
            && failedValueResult.error().code
                == ApplicationErrorCode::RuntimeFailure;
        const bool voidResultsAreValid =
            voidResult.ok()
            && !voidResult.error().has_error()
            && failedVoidResult.failed()
            && failedVoidResult.error().has_error();

        print_result(
            emptyErrorIsValid,
            "empty application error represents success");
        print_result(
            populatedErrorIsValid,
            "application error preserves code and message");
        print_result(
            valueSuccessIsValid,
            "value result exposes successful value");
        print_result(
            valueFailureIsValid,
            "value result exposes failure");
        print_result(
            voidResultsAreValid,
            "void results represent success and failure");

        return emptyErrorIsValid
            && populatedErrorIsValid
            && valueSuccessIsValid
            && valueFailureIsValid
            && voidResultsAreValid;
    }

    [[nodiscard]] bool test_application_state()
    {
        std::cout << "\n=== ApplicationState ===\n";

        ApplicationState state{};

        const bool defaultsAreValid =
            state.phase == ApplicationPhase::Uninitialized
            && !state.quitRequested
            && state.exitCode == 0
            && state.frameIndex == 0;

        state.phase = ApplicationPhase::Running;
        state.frameIndex = 12;
        state.quitRequested = true;
        state.exitCode = 7;

        const bool transitionsAreStored =
            state.phase == ApplicationPhase::Running
            && state.quitRequested
            && state.exitCode == 7
            && state.frameIndex == 12;

        print_result(
            defaultsAreValid,
            "runtime state has safe defaults");
        print_result(
            transitionsAreStored,
            "runtime state stores process-level changes");

        return defaultsAreValid && transitionsAreStored;
    }

    [[nodiscard]] bool test_frame_clock()
    {
        std::cout << "\n=== FrameClock ===\n";

        using namespace std::chrono_literals;

        FrameClock clock{ 0.25 };
        const FrameClock::TimePoint origin{};
        clock.reset(origin);

        const FrameContext first =
            clock.tick(origin + 1s);
        const FrameContext second =
            clock.tick(origin + 1100ms);
        const FrameContext clamped =
            clock.tick(origin + 2s);
        const FrameContext regressed =
            clock.tick(origin + 1500ms);

        const bool firstFrameIsValid =
            first.frameIndex == 0
            && first.firstFrame
            && approximately_equal(first.deltaSeconds, 0.0)
            && approximately_equal(
                first.rawDeltaSeconds,
                0.0)
            && approximately_equal(
                first.totalTimeSeconds,
                1.0);
        const bool regularDeltaIsValid =
            second.frameIndex == 1
            && !second.firstFrame
            && approximately_equal(
                second.rawDeltaSeconds,
                0.1)
            && approximately_equal(
                second.deltaSeconds,
                0.1)
            && approximately_equal(
                second.totalTimeSeconds,
                1.1);
        const bool clampIsValid =
            clamped.frameIndex == 2
            && approximately_equal(
                clamped.rawDeltaSeconds,
                0.9)
            && approximately_equal(
                clamped.deltaSeconds,
                0.25)
            && approximately_equal(
                clamped.totalTimeSeconds,
                2.0);
        const bool regressionIsSafe =
            regressed.frameIndex == 3
            && approximately_equal(
                regressed.rawDeltaSeconds,
                0.0)
            && approximately_equal(
                regressed.deltaSeconds,
                0.0)
            && approximately_equal(
                regressed.totalTimeSeconds,
                2.0);
        const bool nextIndexIsValid =
            clock.next_frame_index() == 4;

        clock.set_maximum_delta(-1.0);
        clock.reset(origin);
        const FrameContext resetFirst = clock.tick(origin);
        const FrameContext zeroClamped =
            clock.tick(origin + 1s);

        const bool resetIsValid =
            resetFirst.frameIndex == 0
            && resetFirst.firstFrame
            && clock.next_frame_index() == 2;
        const bool negativeMaximumIsNormalized =
            approximately_equal(clock.maximum_delta(), 0.0)
            && approximately_equal(
                zeroClamped.rawDeltaSeconds,
                1.0)
            && approximately_equal(
                zeroClamped.deltaSeconds,
                0.0);

        print_result(
            firstFrameIsValid,
            "first frame has zero delta and elapsed total time");
        print_result(
            regularDeltaIsValid,
            "regular frame reports raw and effective delta");
        print_result(
            clampIsValid,
            "large frame delta is clamped");
        print_result(
            regressionIsSafe,
            "regressed timestamp cannot produce negative time");
        print_result(
            nextIndexIsValid,
            "frame indices advance monotonically");
        print_result(
            resetIsValid,
            "reset restarts frame indexing");
        print_result(
            negativeMaximumIsNormalized,
            "negative maximum delta is normalized to zero");

        return firstFrameIsValid
            && regularDeltaIsValid
            && clampIsValid
            && regressionIsSafe
            && nextIndexIsValid
            && resetIsValid
            && negativeMaximumIsNormalized;
    }

} // namespace

int main()
{
    std::cout
        << "=== Locus3D Application Foundation Smoke Test ===\n";

    bool passed = true;
    passed = test_application_config() && passed;
    passed = test_application_error_and_result() && passed;
    passed = test_application_state() && passed;
    passed = test_frame_clock() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All application foundation smoke tests passed ===\n";
        return 0;
    }

    std::cout
        << "=== Application foundation smoke test failed ===\n";
    return 1;
}
