#pragma once

#include "kernel/geometry/mesh/LEM.h"

namespace locus::kernel::modeling {

struct OperationContext {
    geometry::LEM* mesh = nullptr;
    bool validateAfterExecute = true;
    bool rebuildNormals = true;
    bool allowNonManifold = true;

    [[nodiscard]] bool has_mesh() const
    {
        return mesh != nullptr;
    }

    [[nodiscard]] geometry::LEM& editable_mesh()
    {
        return *mesh;
    }

    [[nodiscard]] const geometry::LEM& editable_mesh() const
    {
        return *mesh;
    }
};

}