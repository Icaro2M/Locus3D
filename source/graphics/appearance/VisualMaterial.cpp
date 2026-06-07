#include "graphics/appearance/VisualMaterial.h"

namespace locus::graphics
{
    bool VisualMaterial::is_valid() const
    {
        return shader != nullptr && shader->is_valid();
    }
}