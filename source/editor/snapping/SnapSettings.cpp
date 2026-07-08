/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/SnapSettings.h"

#include <algorithm>

namespace locus::editor {

    SnapMode SnapSettings::modes() const
    {
        return modes_;
    }

    void SnapSettings::set_modes(SnapMode modes)
    {
        modes_ = modes;
    }

    void SnapSettings::enable(SnapMode mode)
    {
        modes_ |= mode;
    }

    void SnapSettings::disable(SnapMode mode)
    {
        modes_ = static_cast<SnapMode>(
            static_cast<std::uint32_t>(modes_) & ~static_cast<std::uint32_t>(mode));
    }

    bool SnapSettings::is_enabled(SnapMode mode) const
    {
        return snappingEnabled_ && has_snap_mode(modes_, mode);
    }

    bool SnapSettings::snapping_enabled() const
    {
        return snappingEnabled_;
    }

    void SnapSettings::set_snapping_enabled(bool enabled)
    {
        snappingEnabled_ = enabled;
    }

    float SnapSettings::grid_size() const
    {
        return gridSize_;
    }

    void SnapSettings::set_grid_size(float size)
    {
        gridSize_ = std::max(size, 0.0001f);
    }

    float SnapSettings::linear_increment() const
    {
        return linearIncrement_;
    }

    void SnapSettings::set_linear_increment(float increment)
    {
        linearIncrement_ = std::max(increment, 0.0001f);
    }

    float SnapSettings::angle_increment() const
    {
        return angleIncrement_;
    }

    void SnapSettings::set_angle_increment(float radians)
    {
        angleIncrement_ = std::max(radians, 0.0001f);
    }

    float SnapSettings::max_distance() const
    {
        return maxDistance_;
    }

    void SnapSettings::set_max_distance(float distance)
    {
        maxDistance_ = std::max(distance, 0.0f);
    }

} // namespace locus::editor