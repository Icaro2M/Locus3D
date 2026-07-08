/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/SnapSolver.h"

namespace locus::editor {

    bool SnapSolver::register_provider(std::unique_ptr<ISnapProvider> provider)
    {
        if (!provider) {
            return false;
        }

        providers_.push_back(std::move(provider));
        return true;
    }

    void SnapSolver::clear()
    {
        providers_.clear();
    }

    std::size_t SnapSolver::provider_count() const
    {
        return providers_.size();
    }

    SnapResult SnapSolver::solve(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        if (!settings.snapping_enabled()) {
            return SnapResult::none();
        }

        const float maxDistance = context.effective_max_distance(settings.max_distance());

        SnapResult best = SnapResult::none();

        for (const std::unique_ptr<ISnapProvider>& provider : providers_) {
            if (!provider || !provider->is_enabled(settings, context)) {
                continue;
            }

            SnapResult result = provider->snap(settings, context);
            if (!result.is_valid()) {
                continue;
            }

            if (result.distance > maxDistance) {
                continue;
            }

            if (!best.is_valid()
                || result.score < best.score
                || (result.score == best.score && result.distance < best.distance)) {
                best = result;
            }
        }

        return best;
    }

} // namespace locus::editor