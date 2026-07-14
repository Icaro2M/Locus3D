/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoController.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/interaction/ToolCancelReason.h"
#include "editor/transform/TransformSession.h"

#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    class SnapSolver;

    /**
     * @brief Input used to begin a transform tool session.
     *
     * The input is independent from platform events. TransformTool converts its
     * normalized ToolEvent into this structure before starting a concrete object
     * or mesh-component session.
     */
    struct TransformToolSessionBeginInput {
        /**
         * @brief Active transform operation.
         */
        GizmoMode mode = GizmoMode::Translate;

        /**
         * @brief World-space orientation of the transform gizmo.
         */
        glm::quat orientation{
            1.0f,
            0.0f,
            0.0f,
            0.0f
        };

        /**
         * @brief Transform space and pivot configuration.
         */
        TransformSessionOptions options{};

        /**
         * @brief Normalized pointer and camera data.
         */
        GizmoPointerInput pointer{};

        /**
         * @brief Optional snapping solver.
         *
         * Snap settings are obtained from ToolContext. The solver remains optional
         * until the editor runtime exposes a shared snapping service.
         */
        const SnapSolver* snapSolver = nullptr;
    };

    /**
     * @brief Input used to update an active transform tool session.
     */
    struct TransformToolSessionUpdateInput {
        /**
         * @brief Normalized pointer and camera data.
         */
        GizmoPointerInput pointer{};

        /**
         * @brief Optional snapping solver override.
         */
        const SnapSolver* snapSolver = nullptr;
    };

    /**
     * @brief Common runtime interface for object and mesh-component transforms.
     *
     * TransformTool selects one concrete session when interaction begins and keeps
     * that session fixed until confirmation or cancellation.
     */
    class ITransformToolSession {
    public:
        virtual ~ITransformToolSession() = default;

        ITransformToolSession() = default;
        ITransformToolSession(
            const ITransformToolSession&) = delete;

        ITransformToolSession& operator=(
            const ITransformToolSession&) = delete;

        ITransformToolSession(
            ITransformToolSession&&) = default;

        ITransformToolSession& operator=(
            ITransformToolSession&&) = default;

        /**
         * @brief Checks whether the session owns an active interaction.
         *
         * @return True when begin succeeded and the session has not ended.
         */
        [[nodiscard]]
        virtual bool is_active() const = 0;

        /**
         * @brief Begins a transform interaction.
         *
         * @param context Tool runtime context.
         * @param input Transform begin input.
         * @return Session start result.
         */
        virtual ToolResult begin(
            ToolContext& context,
            const TransformToolSessionBeginInput& input) = 0;

        /**
         * @brief Updates temporary transform preview.
         *
         * @param context Tool runtime context.
         * @param input Transform update input.
         * @return Preview update result.
         */
        virtual ToolResult update(
            ToolContext& context,
            const TransformToolSessionUpdateInput& input) = 0;

        /**
         * @brief Confirms the current preview through command history.
         *
         * @param context Tool runtime context.
         * @return Commit result.
         */
        virtual ToolResult confirm(
            ToolContext& context) = 0;

        /**
         * @brief Cancels the interaction and restores temporary changes.
         *
         * @param context Tool runtime context.
         * @param reason Cancellation reason.
         * @return Cancellation result.
         */
        virtual ToolResult cancel(
            ToolContext& context,
            ToolCancelReason reason) = 0;

        /**
         * @brief Clears internal state without changing editor data.
         *
         * This method must only be used after the interaction has already been
         * confirmed, cancelled, or otherwise resolved.
         */
        virtual void clear() = 0;
    };

} // namespace locus::editor