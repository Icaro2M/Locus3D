/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/snapping/ISnapProvider.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace locus::editor {

    /**
     * @brief Evaluates registered snap providers and chooses the best result.
     */
    class SnapSolver {
    public:
        /**
         * @brief Registers a snap provider.
         *
         * @param provider Provider to register.
         * @return True when the provider was stored.
         */
        bool register_provider(std::unique_ptr<ISnapProvider> provider);

        /**
         * @brief Removes all registered providers.
         */
        void clear();

        /**
         * @brief Returns the number of registered providers.
         *
         * @return Provider count.
         */
        [[nodiscard]] std::size_t provider_count() const;

        /**
         * @brief Evaluates snapping and returns the best accepted result.
         *
         * @param settings Current snapping settings.
         * @param context Current snapping context.
         * @return Best snap result, or invalid when no provider matched.
         */
        [[nodiscard]] SnapResult solve(
            const SnapSettings& settings,
            const SnapContext& context) const;

    private:
        std::vector<std::unique_ptr<ISnapProvider>> providers_{};
    };

} // namespace locus::editor