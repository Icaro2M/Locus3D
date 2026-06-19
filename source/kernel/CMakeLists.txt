# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_library(locus3d_geometry STATIC)

add_library(Locus3D::Geometry ALIAS locus3d_geometry)

target_sources(locus3d_geometry PRIVATE
    "geometry/mesh/LEM.cpp"
    "modeling/operations/transform/TransformOp.cpp"
    "modeling/operations/face/FlipFaceOp.cpp" "common/Handle.h" "common/Pool.h" "geometry/queries/SelectionHit.h" "geometry/queries/BoundsQuery.h" "geometry/queries/AdjacencyQuery.h" "geometry/topology/TopologyBuilder.h")

target_include_directories(locus3d_geometry PUBLIC
    ${LOCUS3D_SOURCE_DIR}
    ${GLM_DIR}/include
)
