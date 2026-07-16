/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/core/MeshOperationSession.h"

#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/modeling/core/IOperation.h"

#include <utility>

namespace locus::editor {

    MeshOperationSessionState
        MeshOperationSession::state() const
    {
        return state_;
    }

    bool MeshOperationSession::is_active() const
    {
        return state_ != MeshOperationSessionState::Inactive;
    }

    bool MeshOperationSession::has_ready_preview() const
    {
        return state_ == MeshOperationSessionState::PreviewReady &&
            preview_.is_ready();
    }

    bool MeshOperationSession::failed() const
    {
        return state_ == MeshOperationSessionState::Failed;
    }

    const MeshToolTarget&
        MeshOperationSession::target() const
    {
        return target_;
    }

    const kernel::modeling::OperationPreview&
        MeshOperationSession::preview() const
    {
        return preview_;
    }

    const kernel::modeling::GhostMeshBuildOptions&
        MeshOperationSession::preview_options() const
    {
        return previewOptions_;
    }

    void MeshOperationSession::set_preview_options(
        kernel::modeling::GhostMeshBuildOptions options)
    {
        previewOptions_ = std::move(options);

        if (!is_active()) {
            return;
        }

        preview_ =
            kernel::modeling::OperationPreview::invalidated(
                "Preview options changed.");

        state_ = MeshOperationSessionState::Active;
    }

    ToolResult MeshOperationSession::begin(
        const ToolContext& context,
        MeshToolTarget target)
    {
        if (is_active()) {
            return ToolResult::fail(
                "A mesh operation session is already active.");
        }

        if (!target.is_valid()) {
            return ToolResult::fail(
                "Cannot begin a mesh operation with an invalid target.");
        }

        target_ = std::move(target);

        std::string validationMessage;

        if (!resolve_target(context, validationMessage)) {
            clear();

            return ToolResult::fail(
                std::move(validationMessage));
        }

        preview_ =
            kernel::modeling::OperationPreview::invalidated(
                "The mesh operation preview has not been built yet.");

        state_ = MeshOperationSessionState::Active;

        return ToolResult::started(
            EditorDirtyFlags::None,
            "Mesh operation session started.");
    }

    ToolResult MeshOperationSession::rebuild_preview(
        const ToolContext& context,
        kernel::modeling::IOperation& operation)
    {
        if (!is_active()) {
            return ToolResult::fail(
                "Cannot rebuild a preview without an active mesh operation "
                "session.");
        }

        std::string validationMessage;

        const MeshNode* node =
            resolve_target(context, validationMessage);

        if (!node) {
            preview_ =
                kernel::modeling::OperationPreview::failed(
                    validationMessage);

            state_ = MeshOperationSessionState::Failed;

            return ToolResult::fail(
                std::move(validationMessage),
                EditorDirtyFlags::Render);
        }

        preview_ =
            kernel::modeling::GhostMeshBuilder::
            build_operation_preview(
                node->mesh(),
                operation,
                previewOptions_);

        if (preview_.is_failure()) {
            state_ = MeshOperationSessionState::Failed;

            return ToolResult::fail(
                preview_.message().empty()
                ? "Mesh operation preview generation failed."
                : preview_.message(),
                EditorDirtyFlags::Render);
        }

        if (preview_.is_invalidated()) {
            state_ = MeshOperationSessionState::Active;

            return ToolResult::updated(
                EditorDirtyFlags::Render,
                preview_.message().empty()
                ? "Mesh operation preview was invalidated."
                : preview_.message());
        }

        if (preview_.is_empty()) {
            state_ = MeshOperationSessionState::Active;

            return ToolResult::updated(
                EditorDirtyFlags::Render,
                preview_.message().empty()
                ? "Mesh operation produced no preview changes."
                : preview_.message());
        }

        state_ = MeshOperationSessionState::PreviewReady;

        return ToolResult::updated(
            EditorDirtyFlags::Render,
            preview_.message().empty()
            ? "Mesh operation preview rebuilt successfully."
            : preview_.message());
    }

    ToolResult MeshOperationSession::invalidate_preview(
        std::string message)
    {
        if (!is_active()) {
            return ToolResult::ignored();
        }

        if (message.empty()) {
            message = "Mesh operation preview invalidated.";
        }

        preview_ =
            kernel::modeling::OperationPreview::invalidated(
                message);

        state_ = MeshOperationSessionState::Active;

        return ToolResult::updated(
            EditorDirtyFlags::Render,
            std::move(message));
    }

    ToolResult MeshOperationSession::cancel(
        std::string message)
    {
        if (!is_active()) {
            return ToolResult::ignored();
        }

        if (message.empty()) {
            message = "Mesh operation session cancelled.";
        }

        clear();

        return ToolResult::cancelled(
            EditorDirtyFlags::Render,
            std::move(message));
    }

    void MeshOperationSession::clear()
    {
        state_ = MeshOperationSessionState::Inactive;
        target_.clear();
        preview_ = {};
    }

    const MeshNode* MeshOperationSession::resolve_target(
        const ToolContext& context,
        std::string& message) const
    {
        if (!target_.is_valid()) {
            message =
                "The mesh operation target is structurally invalid.";

            return nullptr;
        }

        const MeshNode* node =
            context.scene().find_mesh(target_.nodeId);

        if (!node) {
            message =
                "The mesh node targeted by the operation no longer exists.";

            return nullptr;
        }

        if (!validate_handles(*node, message)) {
            return nullptr;
        }

        return node;
    }

    bool MeshOperationSession::validate_handles(
        const MeshNode& node,
        std::string& message) const
    {
        const kernel::geometry::LEM& mesh =
            node.mesh();

        switch (target_.granularity) {
        case SelectionGranularity::Vertex:
            for (const auto handle : target_.vertices) {
                if (!mesh.is_valid(handle)) {
                    message =
                        "The mesh operation target contains an invalid "
                        "vertex handle.";

                    return false;
                }
            }

            return !target_.vertices.empty();

        case SelectionGranularity::Edge:
            for (const auto handle : target_.edges) {
                if (!mesh.is_valid(handle)) {
                    message =
                        "The mesh operation target contains an invalid "
                        "edge handle.";

                    return false;
                }
            }

            return !target_.edges.empty();

        case SelectionGranularity::Loop:
            for (const auto handle : target_.loops) {
                if (!mesh.is_valid(handle)) {
                    message =
                        "The mesh operation target contains an invalid "
                        "loop handle.";

                    return false;
                }
            }

            return !target_.loops.empty();

        case SelectionGranularity::Face:
            for (const auto handle : target_.faces) {
                if (!mesh.is_valid(handle)) {
                    message =
                        "The mesh operation target contains an invalid "
                        "face handle.";

                    return false;
                }
            }

            return !target_.faces.empty();

        case SelectionGranularity::Object:
            message =
                "Object granularity is not valid for a mesh operation "
                "session.";

            return false;
        }

        message =
            "The mesh operation target uses an unsupported granularity.";

        return false;
    }

} // namespace locus::editor