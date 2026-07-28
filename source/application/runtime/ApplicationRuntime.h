/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/ApplicationConfig.h"
#include "application/ApplicationResult.h"
#include "application/document/DocumentManager.h"
#include "application/input/InputRouter.h"
#include "application/input/InputState.h"
#include "application/runtime/ApplicationState.h"
#include "application/runtime/FrameClock.h"
#include "application/runtime/FrameContext.h"
#include "application/shortcut/ShortcutManager.h"
#include "application/viewport/EditorViewport.h"
#include "application/window/ApplicationWindow.h"

namespace locus::application {

    /**
     * @brief Coordinates application initialization, frames, and shutdown.
     */
    class ApplicationRuntime {
    public:
        /**
         * @brief Creates an uninitialized application runtime.
         */
        ApplicationRuntime() = default;

        /**
         * @brief Shuts down runtime-owned resources.
         */
        ~ApplicationRuntime();

        ApplicationRuntime(const ApplicationRuntime&) = delete;
        ApplicationRuntime& operator=(const ApplicationRuntime&) = delete;
        ApplicationRuntime(ApplicationRuntime&&) = delete;
        ApplicationRuntime& operator=(ApplicationRuntime&&) = delete;

        /**
         * @brief Initializes the window, default document, viewport, and clock.
         *
         * @param config Product-level runtime configuration.
         * @return Success or an initialization error.
         */
        [[nodiscard]] ApplicationResult<void> initialize(
            const ApplicationConfig& config = {});

        /**
         * @brief Runs frames until closure or quit is requested.
         *
         * The runtime shuts down its owned resources before returning.
         *
         * @return Process exit code or a runtime-state error.
         */
        [[nodiscard]] ApplicationResult<int> run();

        /**
         * @brief Executes one main-loop iteration.
         *
         * This method processes input, advances the frame clock, updates the
         * viewport camera, renders the active document, and presents the window.
         *
         * @return Context for the completed frame or a runtime-state error.
         */
        [[nodiscard]] ApplicationResult<FrameContext> run_frame();

        /**
         * @brief Requests an orderly exit from the main loop.
         *
         * @param exitCode Process exit code returned by run().
         */
        void request_quit(int exitCode = 0) noexcept;

        /**
         * @brief Releases runtime-owned resources.
         *
         * Calling this method more than once is safe.
         */
        void shutdown();

        /**
         * @brief Checks whether the runtime is ready to process frames.
         *
         * @return True after successful initialization and before shutdown.
         */
        [[nodiscard]] bool initialized() const noexcept;

        /**
         * @brief Returns the active product configuration.
         *
         * @return Read-only configuration reference.
         */
        [[nodiscard]] const ApplicationConfig& configuration() const noexcept;

        /**
         * @brief Returns process-level runtime state.
         *
         * @return Read-only runtime state reference.
         */
        [[nodiscard]] const ApplicationState& state() const noexcept;

        /**
         * @brief Returns the runtime-owned application window.
         *
         * @return Mutable application window reference.
         */
        [[nodiscard]] ApplicationWindow& window() noexcept;

        /**
         * @brief Returns the runtime-owned application window.
         *
         * @return Read-only application window reference.
         */
        [[nodiscard]] const ApplicationWindow& window() const noexcept;

        /**
         * @brief Returns the runtime-owned document manager.
         *
         * @return Mutable document manager reference.
         */
        [[nodiscard]] DocumentManager& documents() noexcept;

        /**
         * @brief Returns the runtime-owned document manager.
         *
         * @return Read-only document manager reference.
         */
        [[nodiscard]] const DocumentManager& documents() const noexcept;

        /**
         * @brief Returns the runtime-owned primary editor viewport.
         *
         * @return Mutable editor viewport reference.
         */
        [[nodiscard]] EditorViewport& editor_viewport() noexcept;

        /**
         * @brief Returns the runtime-owned primary editor viewport.
         *
         * @return Read-only editor viewport reference.
         */
        [[nodiscard]] const EditorViewport&
            editor_viewport() const noexcept;

        /**
         * @brief Returns current application input state.
         *
         * @return Mutable input state reference.
         */
        [[nodiscard]] InputState& input_state() noexcept;

        /**
         * @brief Returns current application input state.
         *
         * @return Read-only input state reference.
         */
        [[nodiscard]] const InputState& input_state() const noexcept;

        /**
         * @brief Returns application input routing and capture state.
         *
         * @return Mutable input router reference.
         */
        [[nodiscard]] InputRouter& input_router() noexcept;

        /**
         * @brief Returns application input routing and capture state.
         *
         * @return Read-only input router reference.
         */
        [[nodiscard]] const InputRouter& input_router() const noexcept;

        /**
         * @brief Returns semantic shortcut resolver.
         *
         * @return Mutable shortcut manager reference.
         */
        [[nodiscard]] ShortcutManager& shortcut_manager() noexcept;

        /**
         * @brief Returns semantic shortcut resolver.
         *
         * @return Read-only shortcut manager reference.
         */
        [[nodiscard]] const ShortcutManager&
            shortcut_manager() const noexcept;

    private:
        ApplicationConfig configuration_{};
        ApplicationState state_{};
        FrameClock frameClock_{};
        InputState inputState_{};
        InputRouter inputRouter_{};
        ShortcutManager shortcutManager_{};
        ApplicationWindow window_{};
        DocumentManager documents_{};
        EditorViewport editorViewport_{};
    };

} // namespace locus::application
