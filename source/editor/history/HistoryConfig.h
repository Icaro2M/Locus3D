/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstddef>

namespace locus::editor {

    /**
     * @brief Special history limit value that keeps all undo entries.
     *
     * HistoryStack already treats zero as unlimited history. This named constant
     * makes that behavior explicit for callers and configuration code.
     */
    inline constexpr std::size_t UnlimitedHistoryEntries = 0u;

    /**
     * @brief Default number of undo entries retained by the editor.
     *
     * This value is intentionally conservative enough for interactive modeling
     * while still avoiding unbounded memory use by default.
     */
    inline constexpr std::size_t DefaultHistoryMaxEntries = 128u;

    /**
     * @brief Minimum bounded history size accepted by configuration helpers.
     */
    inline constexpr std::size_t MinHistoryMaxEntries = 1u;

    /**
     * @brief Maximum bounded history size accepted by configuration helpers.
     *
     * The editor may store full mesh snapshots for some commands, so keeping this
     * value finite avoids accidental excessive memory usage.
     */
    inline constexpr std::size_t MaxHistoryMaxEntries = 4096u;

    /**
     * @brief Configuration used to initialize or update a HistoryStack.
     */
    struct HistoryConfig {
        /**
         * @brief Maximum number of undo entries retained.
         *
         * Use UnlimitedHistoryEntries to keep all entries.
         */
        std::size_t maxEntries = DefaultHistoryMaxEntries;

        /**
         * @brief Whether new history stacks should start enabled.
         *
         * HistoryStack itself does not currently implement disabled mode. This flag
         * is kept at configuration level for future editor/application integration.
         */
        bool enabled = true;
    };

    /**
     * @brief Clamps a bounded history limit to the supported range.
     *
     * UnlimitedHistoryEntries is preserved as-is.
     *
     * @param maxEntries Requested maximum number of entries.
     * @return Normalized maximum number of entries.
     */
    [[nodiscard]] inline std::size_t normalize_history_max_entries(std::size_t maxEntries)
    {
        if (maxEntries == UnlimitedHistoryEntries) {
            return UnlimitedHistoryEntries;
        }

        return std::clamp(maxEntries, MinHistoryMaxEntries, MaxHistoryMaxEntries);
    }

    /**
     * @brief Returns a normalized history configuration.
     *
     * @param config Input configuration.
     * @return Normalized configuration.
     */
    [[nodiscard]] inline HistoryConfig normalize_history_config(HistoryConfig config)
    {
        config.maxEntries = normalize_history_max_entries(config.maxEntries);
        return config;
    }

} // namespace locus::editor