/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoController.h"
#include "editor/command/CommandResult.h"
#include "editor/tools/transform/ITransformToolSession.h"
#include "editor/tools/transform/MeshTransformTarget.h"

#include <glm/vec3.hpp>

#include <vector>

namespace locus::editor {

    class MeshTransformToolSession final : public ITransformToolSession {
    public:
        MeshTransformToolSession() = default;

        explicit MeshTransformToolSession(
            GizmoController controller);

        [[nodiscard]] bool is_active() const override;

        ToolResult begin(
            ToolContext& context,
            const TransformToolSessionBeginInput& input) override;

        ToolResult update(
            ToolContext& context,
            const TransformToolSessionUpdateInput& input) override;

        ToolResult confirm(
            ToolContext& context) override;

        ToolResult cancel(
            ToolContext& context,
            ToolCancelReason reason) override;

        void clear() override;

        [[nodiscard]] GizmoController& controller();

        [[nodiscard]] const GizmoController& controller() const;

        [[nodiscard]] const MeshTransformTarget& target() const;

        bool preview_translate(
            ToolContext& context,
            const glm::vec3& worldTranslation);

        bool preview_rotate(
            ToolContext& context,
            const glm::quat& worldRotation);

        bool preview_scale(
            ToolContext& context,
            const glm::vec3& worldScale);

    private:
        struct VertexSnapshot {
            kernel::geometry::VertexHandle vertex{};
            glm::vec3 originalPosition{ 0.0f, 0.0f, 0.0f };
            glm::vec3 previewPosition{ 0.0f, 0.0f, 0.0f };
        };

        [[nodiscard]] bool capture_original_positions(
            ToolContext& context);

        [[nodiscard]] bool apply_preview_positions(
            ToolContext& context,
            const std::vector<glm::vec3>& positions);

        [[nodiscard]] bool restore_original_positions(
            ToolContext& context);

        [[nodiscard]] bool has_changes(float epsilon = 0.00001f) const;

        [[nodiscard]] std::vector<glm::vec3> final_positions() const;

        [[nodiscard]] static ToolResult from_command_result(
            CommandResult result);

        GizmoController controller_{};
        MeshTransformTarget target_{};
        std::vector<VertexSnapshot> vertices_{};
        bool active_ = false;
    };

} // namespace locus::editor
