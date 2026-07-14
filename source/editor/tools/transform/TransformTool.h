/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoMode.h"
#include "editor/tools/interaction/DragTool.h"
#include "editor/tools/transform/ITransformToolSession.h"
#include "editor/tools/transform/ObjectTransformToolSession.h"
#include "editor/transform/TransformSession.h"

#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    /**
     * @brief Persistent tool used to translate, rotate, and scale editor targets.
     *
     * One TransformTool instance supports every GizmoMode. The concrete transform
     * session is selected when interaction begins and remains fixed until the
     * interaction is confirmed or cancelled.
     *
     * The current implementation supports complete scene objects through
     * ObjectTransformToolSession. Mesh component transformation will later use a
     * MeshTransformToolSession without introducing another active tool.
     */
    class TransformTool final : public DragTool {
    public:
        /**
         * @brief Stable identifier used by the tool registry.
         */
        static constexpr const char* Id =
            "editor.transform";

        /**
         * @brief Creates an object transform tool in translation mode.
         */
        TransformTool();

        /**
         * @brief Creates an object transform tool with an initial gizmo mode.
         *
         * @param mode Initial transform mode.
         */
        explicit TransformTool(GizmoMode mode);

        /**
         * @brief Returns the default transform tool descriptor.
         *
         * @return Tool descriptor.
         */
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

        /**
         * @brief Returns the active transform mode.
         *
         * @return Current gizmo mode.
         */
        [[nodiscard]]
        GizmoMode mode() const;

        /**
         * @brief Changes the transform mode.
         *
         * The mode cannot be changed during an active interaction.
         *
         * @param mode New transform mode.
         * @return True when the mode was accepted.
         */
        bool set_mode(GizmoMode mode);

        /**
         * @brief Returns the transform session options.
         *
         * @return Read-only session options.
         */
        [[nodiscard]]
        const TransformSessionOptions& options() const;

        /**
         * @brief Changes the transform session options.
         *
         * Options cannot be changed during an active interaction.
         *
         * @param options New transform options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const TransformSessionOptions& options);

        /**
         * @brief Returns the gizmo world orientation.
         *
         * @return Gizmo orientation.
         */
        [[nodiscard]]
        const glm::quat& orientation() const;

        /**
         * @brief Changes the gizmo world orientation.
         *
         * Orientation cannot be changed during an active interaction.
         *
         * @param orientation New normalized orientation.
         * @return True when the orientation was accepted.
         */
        bool set_orientation(
            const glm::quat& orientation);

        /**
         * @brief Returns mutable access to the object transform session.
         *
         * Render adapters may inspect its gizmo state through the controller.
         *
         * @return Object transform session.
         */
        [[nodiscard]]
        ObjectTransformToolSession& object_session();

        /**
         * @brief Returns read-only access to the object transform session.
         *
         * @return Object transform session.
         */
        [[nodiscard]]
        const ObjectTransformToolSession&
            object_session() const;

        /**
         * @brief Returns the gizmo state used for rendering.
         *
         * @return Current gizmo state.
         */
        [[nodiscard]]
        const GizmoState& gizmo_state() const;

    protected:
        /**
         * @brief Checks whether the tool can activate.
         *
         * The tool currently requires object editor mode. A selection is not
         * required for activation because the tool may remain active while the
         * selection changes.
         *
         * @param context Tool context.
         * @return True in object editor mode.
         */
        [[nodiscard]]
        bool can_activate_tool(
            const ToolContext& context) const override;

        /**
         * @brief Prepares gizmo state when the tool activates.
         *
         * @param context Tool context.
         * @return Activation result.
         */
        ToolResult on_activate(
            ToolContext& context) override;

        /**
         * @brief Clears transient gizmo state on deactivation.
         *
         * @param context Tool context.
         * @return Deactivation result.
         */
        ToolResult on_deactivate(
            ToolContext& context) override;

        /**
         * @brief Updates gizmo hover before a drag begins.
         *
         * @param context Tool context.
         * @param event Pointer movement event.
         * @return Hover result.
         */
        ToolResult on_pointer_hover(
            ToolContext& context,
            const ToolEvent& event) override;

        /**
         * @brief Begins an object transform session.
         *
         * @param context Tool context.
         * @param event Pointer press event.
         * @return Session start result.
         */
        ToolResult on_begin_drag(
            ToolContext& context,
            const ToolEvent& event) override;

        /**
         * @brief Updates the active transform preview.
         *
         * @param context Tool context.
         * @param event Pointer movement event.
         * @return Preview update result.
         */
        ToolResult on_update_drag(
            ToolContext& context,
            const ToolEvent& event) override;

        /**
         * @brief Handles pointer release before automatic confirmation.
         *
         * @param context Tool context.
         * @param event Pointer release event.
         * @return Release result.
         */
        ToolResult on_release_drag(
            ToolContext& context,
            const ToolEvent& event) override;

        /**
         * @brief Commits the active transform session.
         *
         * @param context Tool context.
         * @return Commit result.
         */
        ToolResult on_confirm_drag(
            ToolContext& context) override;

        /**
         * @brief Cancels and restores the active transform session.
         *
         * @param context Tool context.
         * @param reason Cancellation reason.
         * @return Cancellation result.
         */
        ToolResult on_cancel_drag(
            ToolContext& context,
            ToolCancelReason reason) override;

    private:
        /**
         * @brief Converts normalized tool pointer data to gizmo input.
         *
         * @param event Source tool event.
         * @return Gizmo pointer input.
         */
        [[nodiscard]]
        static GizmoPointerInput make_gizmo_pointer(
            const ToolEvent& event);

        /**
         * @brief Resolves the current object selection pivot.
         *
         * @param context Tool context.
         * @return World-space pivot.
         */
        [[nodiscard]]
        glm::vec3 resolve_object_pivot(
            const ToolContext& context) const;

        /**
         * @brief Returns the concrete session used by the current implementation.
         *
         * @return Active session interface.
         */
        [[nodiscard]]
        ITransformToolSession* active_session();

        /**
         * @brief Returns the concrete session used by the current implementation.
         *
         * @return Read-only active session interface.
         */
        [[nodiscard]]
        const ITransformToolSession*
            active_session() const;

        GizmoMode mode_ = GizmoMode::Translate;

        TransformSessionOptions options_{};

        glm::quat orientation_{
            1.0f,
            0.0f,
            0.0f,
            0.0f
        };

        ObjectTransformToolSession objectSession_{};

        ITransformToolSession* activeSession_ = nullptr;
    };

} // namespace locus::editor