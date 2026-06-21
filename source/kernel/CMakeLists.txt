# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_library(locus3d_geometry STATIC)

add_library(Locus3D::Geometry ALIAS locus3d_geometry)

target_sources(locus3d_geometry PRIVATE
    "geometry/mesh/LEM.cpp"
    "modeling/operations/transform/TransformOp.cpp"
    "modeling/operations/face/FlipFaceOp.cpp"
    "common/Handle.h"
    "common/Pool.h"
    "geometry/queries/SelectionHit.h"
    "geometry/queries/BoundsQuery.h"
    "geometry/queries/AdjacencyQuery.h"
    "geometry/topology/TopologyBuilder.h"
    "geometry/queries/ProximityQuery.h"
    "geometry/queries/RaycastQuery.h"
    "geometry/queries/SelectionQuery.h"
    "geometry/spatial/BVH.h"
    "geometry/spatial/BVH.cpp"
    "geometry/spatial/BVHBuilder.h"
    "geometry/spatial/BVHQuery.h"
    "geometry/spatial/SpatialIndex.h"
    "geometry/primitives/IPrimitiveBuilder.h"
    "geometry/primitives/CylinderBuilder.h"
    "geometry/primitives/SphereBuilder.h"
    "geometry/primitives/ConeBuilder.h"
    "geometry/primitives/TorusBuilder.h"
    "geometry/primitives/PrimitiveRegistry.h"  "validation/core/ValidationSeverity.h" "validation/core/ValidationIssue.h" "validation/core/ValidationReport.h" "validation/core/ValidationContext.h" "validation/core/IValidationCheck.h" "validation/pipeline/ValidationMode.h" "validation/pipeline/ValidationPipeline.h" "validation/checks/lem/HandleValidityCheck.h" "validation/checks/lem/FaceCycleCheck.h" "validation/checks/lem/RadialCycleCheck.h" "validation/checks/lem/ElementReferenceCheck.h")

target_include_directories(locus3d_geometry PUBLIC
    ${LOCUS3D_SOURCE_DIR}
    ${GLM_DIR}/include
)
