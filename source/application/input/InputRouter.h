/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/input/InputCapture.h"

namespace locus::application {

    class InputState;

    /**
     * @brief Top-level destination selected for an input frame.
     */
    enum class InputRouteDestination {
        None,
        ViewportCamera,
        Editor,
        Shortcut
    };

    /**
     * @brief Viewport navigation operation requested by routing.
     */
    enum class ViewportNavigationAction {
        None,
        Orbit,
        Pan,
        Zoom
    };

    /**
     * @brief Routed interpretation of one normalized input frame.
     */
    struct InputRouteResult {
        InputRouteDestination destination = InputRouteDestination::None;
        InputCaptureOwner owner = InputCaptureOwner::None;
        ViewportNavigationAction navigation =
            ViewportNavigationAction::None;
        InputVector2 cursorDelta{};
        double scrollDelta = 0.0;
        bool captureBegan = false;
        bool captureEnded = false;
        bool editorPointerMove = false;
        bool editorPointerPress = false;
        bool editorPointerRelease = false;
    };

    /**
     * @brief Selects the application consumer for current pointer input.
     */
    class InputRouter {
    public:
        /**
         * @brief Routes one input frame to a semantic destination.
         *
         * Camera gestures are middle-button drag for orbit, Shift+middle drag
         * for pan, and vertical scroll for zoom. Plain left input is captured
         * for the editor tool path and does not affect the camera.
         *
         * @param input Current input state after platform polling.
         * @return Route result with destination and semantic input intent.
         */
        [[nodiscard]] InputRouteResult route(const InputState& input);

        /**
         * @brief Cancels any active routing and pointer capture.
         */
        void reset() noexcept;

        [[nodiscard]] const InputCapture& capture() const noexcept;
        [[nodiscard]] InputCaptureOwner routed_owner() const noexcept;

    private:
        enum class CameraDragMode {
            None,
            Orbit,
            Pan
        };

        [[nodiscard]] bool begin_capture(
            const InputState& input,
            InputRouteResult& result);
        void route_active_capture(
            const InputState& input,
            InputRouteResult& result) const;
        void route_scroll(
            const InputState& input,
            InputRouteResult& result) const;
        void finish_released_capture(
            const InputState& input,
            InputRouteResult& result);

    private:
        InputCapture capture_{};
        CameraDragMode cameraDragMode_ = CameraDragMode::None;
        InputCaptureOwner routedOwner_ = InputCaptureOwner::None;
    };

} // namespace locus::application
