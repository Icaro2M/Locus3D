/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/input/InputEvent.h"

namespace locus::application {

    /**
     * @brief Logical owner of an active pointer interaction.
     */
    enum class InputCaptureOwner {
        None,
        ViewportCamera,
        EditorTool
    };

    /**
     * @brief Preserves pointer ownership from press through release.
     */
    class InputCapture {
    public:
        /**
         * @brief Acquires pointer ownership when no capture is active.
         *
         * @param owner Requested non-empty owner.
         * @param button Button that begins and ends the interaction.
         * @return True when capture was acquired.
         */
        [[nodiscard]] bool acquire(
            InputCaptureOwner owner,
            MouseButton button) noexcept
        {
            if (active()
                || owner == InputCaptureOwner::None
                || button == MouseButton::Unknown) {
                return false;
            }

            owner_ = owner;
            button_ = button;
            return true;
        }

        /**
         * @brief Releases capture when its initiating button is released.
         *
         * @param button Released button.
         * @return True when an active capture ended.
         */
        [[nodiscard]] bool release_for(MouseButton button) noexcept
        {
            if (!active() || button != button_) {
                return false;
            }

            cancel();
            return true;
        }

        /**
         * @brief Cancels capture regardless of its current owner.
         */
        void cancel() noexcept
        {
            owner_ = InputCaptureOwner::None;
            button_ = MouseButton::Unknown;
        }

        [[nodiscard]] bool active() const noexcept
        {
            return owner_ != InputCaptureOwner::None;
        }

        [[nodiscard]] InputCaptureOwner owner() const noexcept
        {
            return owner_;
        }

        [[nodiscard]] MouseButton button() const noexcept
        {
            return button_;
        }

    private:
        InputCaptureOwner owner_ = InputCaptureOwner::None;
        MouseButton button_ = MouseButton::Unknown;
    };

} // namespace locus::application
