/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ITool.h"
#include "editor/tools/core/ToolId.h"

#include <memory>
#include <utility>

namespace locus::editor {

    /**
     * @brief Owning record for the currently active editor tool.
     *
     * The tool manager uses this type to keep the active tool identifier and
     * instance synchronized.
     */
    struct ActiveTool {
        /**
         * @brief Stable identifier of the active tool.
         */
        ToolId id{};

        /**
         * @brief Owned active tool instance.
         */
        std::unique_ptr<ITool> instance{};

        /**
         * @brief Creates an empty active tool record.
         */
        ActiveTool() = default;

        /**
         * @brief Creates an active tool record.
         *
         * @param id Stable tool identifier.
         * @param instance Owned tool instance.
         */
        ActiveTool(
            ToolId id,
            std::unique_ptr<ITool> instance)
            : id(std::move(id)),
            instance(std::move(instance)) {
        }

        ActiveTool(const ActiveTool&) = delete;
        ActiveTool& operator=(const ActiveTool&) = delete;
        ActiveTool(ActiveTool&&) noexcept = default;
        ActiveTool& operator=(ActiveTool&&) noexcept = default;

        /**
         * @brief Checks whether this record contains a valid tool.
         *
         * @return True when both identifier and instance are valid.
         */
        [[nodiscard]] bool is_valid() const {
            return id.is_valid() && instance != nullptr;
        }

        /**
         * @brief Checks whether this record is empty or invalid.
         *
         * @return True when no valid tool is stored.
         */
        [[nodiscard]] bool is_empty() const {
            return !is_valid();
        }

        /**
         * @brief Clears the active tool record.
         */
        void clear() {
            instance.reset();
            id = {};
        }
    };

} // namespace locus::editor