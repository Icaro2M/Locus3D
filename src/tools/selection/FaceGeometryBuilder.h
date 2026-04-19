#pragma once

#include "FaceSelection.h"
#include "FaceGeometry.h"

class FaceGeometryBuilder
{
public:
    FaceGeometry build(const FaceSelection& selection) const;
};