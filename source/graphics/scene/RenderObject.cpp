/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/scene/RenderObject.h"

namespace locus::graphics
{
    bool RenderObject::is_drawable() const
    {
        // Keep renderer-side validation cheap and explicit before issuing GPU calls.
        return visibility.visible
            && mesh != nullptr
            && shader != nullptr
            && mesh->is_valid()
            && shader->is_valid();
    }
}
