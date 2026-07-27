/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/input/InputCapture.h"

namespace locus::application {

    class EditorViewport;
    class InputState;

    /**
     * @brief Selects the application consumer for current pointer input.
     */
    class InputRouter {
    public:
        /**
         * @brief Routes one input frame to camera or future editor tools.
         *
         * Camera gestures are Alt+left drag for orbit, Alt+middle drag for pan,
         * and vertical scroll for zoom. Plain left drag is captured for the
         * future editor tool path and does not affect the camera.
         *
         * @param input Current input state after platform polling.
         * @param viewport Primary editor viewport.
         */
        void route(const InputState& input, EditorViewport& viewport);

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

        [[nodiscard]] bool begin_capture(const InputState& input);
        void route_camera_drag(
            const InputState& input,
            EditorViewport& viewport);
        void finish_released_capture(const InputState& input);

    private:
        InputCapture capture_{};
        CameraDragMode cameraDragMode_ = CameraDragMode::None;
        InputCaptureOwner routedOwner_ = InputCaptureOwner::None;
    };

} // namespace locus::application
