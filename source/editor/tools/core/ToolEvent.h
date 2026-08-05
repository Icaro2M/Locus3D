/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "graphics/picking/PickingId.h"

namespace locus::editor {

    /**
     * @brief Semantic event types accepted by editor tools.
     *
     * The application layer converts platform-specific input into these normalized
     * events before forwarding them to the editor.
     */
    enum class ToolEventType {
        /**
         * @brief Pointer moved inside the active viewport.
         */
        PointerMove,

        /**
         * @brief Pointer button was pressed.
         */
        PointerPress,

        /**
         * @brief Pointer button was released.
         */
        PointerRelease,

        /**
         * @brief Pointer wheel or equivalent scroll input changed.
         */
        Scroll,

        /**
         * @brief Current tool interaction should be confirmed.
         */
        Confirm,

        /**
         * @brief Current tool interaction should be cancelled.
         */
        Cancel,

        /**
         * @brief Editor update step used for coalesced per-frame work.
         */
        Update,

        /**
         * @brief Viewport or application focus was lost.
         */
        FocusLost
    };

    /**
     * @brief Normalized pointer buttons used by editor tools.
     */
    enum class ToolPointerButton {
        /**
         * @brief No pointer button.
         */
        None,

        /**
         * @brief Primary selection or interaction button.
         */
        Primary,

        /**
         * @brief Secondary or contextual interaction button.
         */
        Secondary,

        /**
         * @brief Auxiliary pointer button.
         */
        Auxiliary
    };

    /**
     * @brief Modifier flags attached to normalized editor input.
     */
    enum class ToolModifiers : std::uint32_t {
        /**
         * @brief No modifier is active.
         */
        None = 0u,

        /**
         * @brief Additive or shift-like modifier.
         */
        Additive = 1u << 0u,

        /**
         * @brief Toggle or control-like modifier.
         */
        Toggle = 1u << 1u,

        /**
         * @brief Alternate interaction modifier.
         */
        Alternate = 1u << 2u,

        /**
         * @brief Precision interaction modifier.
         */
        Precision = 1u << 3u
    };

    /**
     * @brief Combines two tool modifier masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Combined modifier mask.
     */
    [[nodiscard]] constexpr ToolModifiers operator|(
        ToolModifiers lhs,
        ToolModifiers rhs) {

        return static_cast<ToolModifiers>(
            static_cast<std::uint32_t>(lhs) |
            static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Intersects two tool modifier masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Intersected modifier mask.
     */
    [[nodiscard]] constexpr ToolModifiers operator&(
        ToolModifiers lhs,
        ToolModifiers rhs) {

        return static_cast<ToolModifiers>(
            static_cast<std::uint32_t>(lhs) &
            static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Adds modifiers to an existing mask.
     *
     * @param lhs Mask to update.
     * @param rhs Modifiers to add.
     * @return Updated modifier mask.
     */
    constexpr ToolModifiers& operator|=(
        ToolModifiers& lhs,
        ToolModifiers rhs) {

        lhs = lhs | rhs;
        return lhs;
    }

    /**
     * @brief Checks whether a modifier mask contains a given modifier.
     *
     * @param mask Mask to inspect.
     * @param modifier Modifier to test.
     * @return True when the modifier is present.
     */
    [[nodiscard]] constexpr bool has_modifier(
        ToolModifiers mask,
        ToolModifiers modifier) {

        return static_cast<std::uint32_t>(
            mask & modifier) != 0u;
    }

    /**
     * @brief World-space ray supplied by the application for viewport interaction.
     */
    struct ToolPointerRay {
        /**
         * @brief Ray origin in world coordinates.
         */
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Ray direction in world coordinates.
         */
        glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
    };

    /**
     * @brief Normalized pointer and camera data attached to a tool event.
     */
    struct ToolPointerData {
        /**
         * @brief Pointer position relative to the active viewport.
         */
        glm::vec2 viewportPosition{ 0.0f, 0.0f };

        /**
         * @brief Pointer movement since the previous normalized event.
         */
        glm::vec2 viewportDelta{ 0.0f, 0.0f };

        glm::vec2 viewportSize{ 0.0f, 0.0f };

        glm::mat4 viewProjection{ 1.0f };

        /**
         * @brief Compact picking identifier sampled at the pointer position.
         *
         * The application layer reads this value from the graphics picking buffer.
         * The editor resolves it into a SceneNodeId through PickingSync.
         */
        graphics::PickingId pickingId =
            graphics::PickingId::invalid();

        std::vector<graphics::PickingId> regionalPickingIds{};

        /**
         * @brief Checks whether the pointer has a valid picking sample.
         *
         * @return True when the sampled picking identifier is valid.
         */
        [[nodiscard]] bool has_picking_hit() const {
            return pickingId.is_valid();
        }

        /**
         * @brief World-space ray passing through the pointer position.
         */
        ToolPointerRay worldRay{};

        /**
         * @brief Camera view direction in world coordinates.
         */
        glm::vec3 viewDirection{ 0.0f, 0.0f, -1.0f };

        /**
         * @brief Camera right direction in world coordinates.
         */
        glm::vec3 viewRight{ 1.0f, 0.0f, 0.0f };

        /**
         * @brief Camera up direction in world coordinates.
         */
        glm::vec3 viewUp{ 0.0f, 1.0f, 0.0f };

        /**
         * @brief Scale suitable for world-space interaction handles.
         */
        float visualScale = 1.0f;

        /**
        * @brief Clears the current graphics picking sample.
        */
        void clear_picking_hit() {
            pickingId = graphics::PickingId::invalid();
        }
    };

    /**
     * @brief Platform-independent input event consumed by editor tools.
     *
     * Physical keys, GLFW constants, ImGui state, and operating-system event
     * objects must not cross this boundary. The application layer is responsible
     * for mapping raw input to semantic tool events.
     */
    struct ToolEvent {
        /**
         * @brief Semantic event type.
         */
        ToolEventType type = ToolEventType::Update;

        /**
         * @brief Pointer button associated with press or release events.
         */
        ToolPointerButton button = ToolPointerButton::None;

        /**
         * @brief Semantic modifiers active for this event.
         */
        ToolModifiers modifiers = ToolModifiers::None;

        /**
         * @brief Pointer and camera information for viewport interactions.
         */
        ToolPointerData pointer{};

        /**
         * @brief Signed scroll amount for scroll events.
         */
        float scrollDelta = 0.0f;

        /**
         * @brief Elapsed time in seconds for update events.
         */
        float deltaSeconds = 0.0f;

        /**
         * @brief Checks whether this is a pointer-related event.
         *
         * @return True for move, press, release, and scroll events.
         */
        [[nodiscard]] bool is_pointer_event() const {
            return
                type == ToolEventType::PointerMove ||
                type == ToolEventType::PointerPress ||
                type == ToolEventType::PointerRelease ||
                type == ToolEventType::Scroll;
        }

        /**
         * @brief Checks whether the primary pointer button is associated with this
         * event.
         *
         * @return True when the event uses the primary button.
         */
        [[nodiscard]] bool uses_primary_button() const {
            return button == ToolPointerButton::Primary;
        }

        /**
         * @brief Checks whether a semantic modifier is active.
         *
         * @param modifier Modifier to test.
         * @return True when the modifier is active.
         */
        [[nodiscard]] bool has_modifier(
            ToolModifiers modifier) const {

            return locus::editor::has_modifier(
                modifiers,
                modifier);
        }
    };

} // namespace locus::editor
