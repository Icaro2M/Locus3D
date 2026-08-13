/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/document/DocumentSession.h"

#include "editor/actions/Actions.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/RegisterBuiltinTools.h"
#include "editor/tools/selection/SelectTool.h"

namespace locus::application {

    DocumentSession::DocumentSession(DocumentId id)
        : id_(id)
        , commandDispatcher_(editor_)
        , actionExecutor_(actionRegistry_)
        , toolManager_(toolRegistry_)
    {
        (void)editor::register_default_actions(
            actionRegistry_);

        (void)editor::register_builtin_tools(
            toolRegistry_);

        editor::ToolContext toolContext(
            editor_,
            commandDispatcher_,
            history_,
            editorSync_.picking_sync());

        (void)toolManager_.activate_tool(
            toolContext,
            editor::ToolId{ editor::SelectTool::Id });
    }

    DocumentId DocumentSession::id() const noexcept
    {
        return id_;
    }

    editor::Editor& DocumentSession::editor() noexcept
    {
        return editor_;
    }

    const editor::Editor& DocumentSession::editor() const noexcept
    {
        return editor_;
    }

    editor::CommandDispatcher&
    DocumentSession::command_dispatcher() noexcept
    {
        return commandDispatcher_;
    }

    const editor::CommandDispatcher&
    DocumentSession::command_dispatcher() const noexcept
    {
        return commandDispatcher_;
    }

    editor::HistoryStack& DocumentSession::history() noexcept
    {
        return history_;
    }

    const editor::HistoryStack& DocumentSession::history() const noexcept
    {
        return history_;
    }

    editor::ActionRegistry& DocumentSession::action_registry() noexcept
    {
        return actionRegistry_;
    }

    const editor::ActionRegistry&
    DocumentSession::action_registry() const noexcept
    {
        return actionRegistry_;
    }

    editor::ActionExecutor& DocumentSession::action_executor() noexcept
    {
        return actionExecutor_;
    }

    const editor::ActionExecutor&
    DocumentSession::action_executor() const noexcept
    {
        return actionExecutor_;
    }

    editor::ToolRegistry& DocumentSession::tool_registry() noexcept
    {
        return toolRegistry_;
    }

    const editor::ToolRegistry&
    DocumentSession::tool_registry() const noexcept
    {
        return toolRegistry_;
    }

    editor::ToolManager& DocumentSession::tool_manager() noexcept
    {
        return toolManager_;
    }

    const editor::ToolManager&
    DocumentSession::tool_manager() const noexcept
    {
        return toolManager_;
    }

    editor::EditorSync& DocumentSession::editor_sync() noexcept
    {
        return editorSync_;
    }

    const editor::EditorSync&
    DocumentSession::editor_sync() const noexcept
    {
        return editorSync_;
    }

    bool DocumentSession::is_dirty() const noexcept
    {
        return externalDirty_ || !history_.is_clean();
    }

    bool DocumentSession::has_path() const noexcept
    {
        return !path_.empty();
    }

    const std::filesystem::path& DocumentSession::path() const noexcept
    {
        return path_;
    }

    std::string DocumentSession::display_name() const
    {
        if (!has_path()) {
            return "Untitled";
        }

        const std::string fileName = path_.filename().string();
        return fileName.empty() ? path_.string() : fileName;
    }

    void DocumentSession::mark_dirty() noexcept
    {
        externalDirty_ = true;
    }

    void DocumentSession::mark_history_changed() noexcept
    {
    }

    void DocumentSession::mark_saved(
        const std::filesystem::path& path)
    {
        path_ = path;
        history_.mark_clean();
        externalDirty_ = false;
    }

    void DocumentSession::mark_loaded(
        const std::filesystem::path& path)
    {
        path_ = path;
        history_.clear();
        history_.mark_clean();
        externalDirty_ = false;
    }

    void DocumentSession::clear_path()
    {
        path_.clear();
    }

} // namespace locus::application
