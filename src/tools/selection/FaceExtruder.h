#pragma once

#include "FaceSelection.h"
#include "FaceGeometryBuilder.h"

class FaceExtruder
{
private:
    FaceGeometryBuilder m_FaceGeometryBuilder;

public:
    bool extrude(FaceSelection& selection, float distance);
};