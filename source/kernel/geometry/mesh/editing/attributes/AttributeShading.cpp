/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/attributes/AttributeShading.h"

#include "kernel/geometry/mesh/LEM.h"

#include <algorithm>

namespace locus::kernel::geometry {

    AttributeShading::AttributeShading(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    bool AttributeShading::set_smooth(EdgeHandle handle, bool smooth)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Edge& edge = mesh_.edge(handle);
        if (edge.smooth == smooth) {
            return true;
        }

        edge.smooth = smooth;
        diff_.record(LEMChangeType::EdgeModified, handle);
        return true;
    }

    bool AttributeShading::set_crease(EdgeHandle handle, float crease)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        const float clampedCrease = std::clamp(crease, 0.0f, 1.0f);

        Edge& edge = mesh_.edge(handle);
        if (edge.crease == clampedCrease) {
            return true;
        }

        edge.crease = clampedCrease;
        diff_.record(LEMChangeType::EdgeModified, handle);
        return true;
    }

}