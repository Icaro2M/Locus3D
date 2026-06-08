/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/appearance/VisualMaterial.h"

namespace locus::graphics
{
    bool VisualMaterial::is_valid() const
    {
        return shader != nullptr && shader->is_valid();
    }
}
